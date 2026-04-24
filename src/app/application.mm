// application.mm — Application implementation (Objective-C++, ARC enabled)
// This file is the only place that directly uses Metal / GLFW / ImGui setup.
// All ObjC types are hidden behind Application::Impl (PIMPL).

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include "implot.h"

#include "fastscope/application.hpp"
#include "fastscope/app_state.hpp"
#include "fastscope/log.hpp"
#include "fastscope/version.hpp"
#include "fastscope/simulation_list_ui.hpp"
#include "fastscope/signal_tree_widget.hpp"
#include "fastscope/plot_engine.hpp"
#include "fastscope/simulation_file.hpp"
#include "fastscope/color_manager.hpp"
#include "fastscope/file_open_dialog.hpp"
#include "fastscope/csv_exporter.hpp"
#include "fastscope/png_exporter.hpp"
#include "fastscope/tool_manager.hpp"
#include "fastscope/derived_signal.hpp"
#include "fastscope/operation_registry.hpp"
#include "fastscope/signal_metadata.hpp"
#include "fastscope/log_pane.hpp"
#include "fastscope/session.hpp"

#include <mach/mach.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>

namespace fastscope {

// ── System stats (macOS) ───────────────────────────────────────────────────────
struct SystemStats {
    float cpu_pct = 0.0f;   ///< App CPU usage in percent.
    float rss_mb  = 0.0f;   ///< Resident set size in megabytes.
};

/// Returns lightweight per-frame system metrics.
/// CPU% is computed from rusage delta so it reflects actual usage since last call.
static SystemStats query_system_stats() noexcept
{
    SystemStats s;

    // ── Memory: task resident set size ──────────────────────────────────────────
    {
        mach_task_basic_info_data_t info{};
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                      reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
            s.rss_mb = static_cast<float>(info.resident_size) / (1024.0f * 1024.0f);
    }

    // ── CPU: rusage delta / wall-clock delta ─────────────────────────────────
    {
        static struct rusage s_prev{};
        static struct timeval s_wall_prev{};
        static bool s_first = true;

        struct rusage cur{};
        struct timeval wall_now{};
        getrusage(RUSAGE_SELF, &cur);
        gettimeofday(&wall_now, nullptr);

        if (!s_first) {
            // CPU seconds consumed since last sample
            double du = (cur.ru_utime.tv_sec  - s_prev.ru_utime.tv_sec) +
                        (cur.ru_utime.tv_usec - s_prev.ru_utime.tv_usec) * 1e-6;
            double ds = (cur.ru_stime.tv_sec  - s_prev.ru_stime.tv_sec) +
                        (cur.ru_stime.tv_usec - s_prev.ru_stime.tv_usec) * 1e-6;
            double dw = (wall_now.tv_sec  - s_wall_prev.tv_sec) +
                        (wall_now.tv_usec - s_wall_prev.tv_usec) * 1e-6;
            if (dw > 0.0)
                s.cpu_pct = static_cast<float>((du + ds) / dw * 100.0);
        }

        s_prev      = cur;
        s_wall_prev = wall_now;
        s_first     = false;
    }

    return s;
}

// ── forward declarations ───────────────────────────────────────────────────────
static void render_ui_frame(AppState& state, PlotEngine& engine, const ImGuiIO& io);
static void draw_vertical_splitter(const char* id, float* width, float avail_w);
// ── PIMPL — holds all ObjC / Metal / GLFW state ───────────────────────────────

struct Application::Impl {
    GLFWwindow*              window       = nullptr;
    id<MTLDevice>            device;
    id<MTLCommandQueue>      commandQueue;
    CAMetalLayer*            metalLayer;
    MTLRenderPassDescriptor* rpd;

    AppState state;
    bool     initialized = false;
    Config   config;
    PlotEngine engine;
};

// ── Lifecycle ──────────────────────────────────────────────────────────────────

Application::Application() : m_impl(std::make_unique<Impl>()) {}

Application::~Application()
{
    if (m_impl && m_impl->initialized)
        shutdown();
}

bool Application::init()           { return init(Config{}); }
bool Application::init(Config cfg)
{
    m_impl->config = cfg;

    // ── Logging ───────────────────────────────────────────────────────────────
    fastscope::log::init({
        .file_path = "fastscope.log",
        .console   = true,
        .file      = false,  // file log opt-in: avoid polluting cwd during dev
        .level     = spdlog::level::debug,
    });
    FASTSCOPE_LOG_INFO("FastScope {} starting up", fastscope::version());

    // ── GLFW ──────────────────────────────────────────────────────────────────
    glfwSetErrorCallback([](int error, const char* description) {
        FASTSCOPE_LOG_ERROR("GLFW error {}: {}", error, description);
    });

    if (!glfwInit()) {
        FASTSCOPE_LOG_ERROR("Failed to initialise GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API,               GLFW_NO_API);   // Metal, not GL
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE,                GLFW_TRUE);

    const std::string title{cfg.title};
    m_impl->window = glfwCreateWindow(cfg.width, cfg.height, title.c_str(),
                                      nullptr, nullptr);
    if (!m_impl->window) {
        FASTSCOPE_LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    // ── GLFW window user pointer (for drop callback) ───────────────────────────
    glfwSetWindowUserPointer(m_impl->window, m_impl.get());

    // ── Drag-and-drop: accept .h5 / .hdf5 files from Finder ──────────────────
    glfwSetDropCallback(m_impl->window, [](GLFWwindow* w, int count, const char** paths) {
        auto* impl = static_cast<Application::Impl*>(glfwGetWindowUserPointer(w));
        if (!impl) return;
        static int drop_counter = 0;
        for (int i = 0; i < count; ++i) {
            std::filesystem::path p(paths[i]);
            std::string ext = p.extension().string();
            // case-insensitive extension check
            for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (ext != ".h5" && ext != ".hdf5") {
                FASTSCOPE_LOG_WARN("Dropped file ignored (not .h5/.hdf5): {}", paths[i]);
                continue;
            }
            const std::string id = "drop" + std::to_string(drop_counter++);
            auto sim = std::make_unique<SimulationFile>(id);
            if (auto res = sim->open(p); res) {
                FASTSCOPE_LOG_INFO("Dropped file opened: {}", paths[i]);
                impl->state.simulations.push_back(std::move(sim));
            } else {
                FASTSCOPE_LOG_ERROR("Drop open failed for {}: {}", paths[i], res.error().message);
            }
        }
    });

    // ── Metal ─────────────────────────────────────────────────────────────────
    m_impl->device = MTLCreateSystemDefaultDevice();
    if (!m_impl->device) {
        FASTSCOPE_LOG_ERROR("Metal is not supported on this device");
        glfwDestroyWindow(m_impl->window);
        glfwTerminate();
        return false;
    }
    m_impl->commandQueue = [m_impl->device newCommandQueue];

    // ── CAMetalLayer → attach to GLFW NSView ──────────────────────────────────
    NSWindow* nswindow    = glfwGetCocoaWindow(m_impl->window);
    NSView*   contentView = [nswindow contentView];
    [contentView setWantsLayer:YES];

    m_impl->metalLayer                 = [CAMetalLayer layer];
    m_impl->metalLayer.device          = m_impl->device;
    m_impl->metalLayer.pixelFormat     = MTLPixelFormatBGRA8Unorm;
    m_impl->metalLayer.framebufferOnly = YES;
    m_impl->metalLayer.contentsScale   = [nswindow backingScaleFactor];
    [contentView setLayer:m_impl->metalLayer];

    // ── Dear ImGui ────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (cfg.enable_docking)   io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (cfg.enable_viewports) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiStyle& style                       = ImGui::GetStyle();
        style.WindowRounding                    = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w       = 1.0f;
    }

    ImGui_ImplGlfw_InitForOther(m_impl->window, /*install_callbacks=*/true);
    ImGui_ImplMetal_Init(m_impl->device);

    // ── Render pass descriptor (reused each frame; texture swapped per frame) ─
    m_impl->rpd = [MTLRenderPassDescriptor new];
    m_impl->rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
    m_impl->rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
    m_impl->rpd.colorAttachments[0].clearColor  = MTLClearColorMake(0.10, 0.10, 0.12, 1.0);

    m_impl->initialized = true;
    FASTSCOPE_LOG_INFO("Application initialised");

    // Auto-restore last session (silently fails if no session file exists)
    load_session(m_impl->state);

    return true;
}

// ── Main loop ──────────────────────────────────────────────────────────────────

void Application::run()
{
    if (!m_impl->initialized) return;

    ImGuiIO& io = ImGui::GetIO();

    while (!glfwWindowShouldClose(m_impl->window) && !m_impl->state.quit_requested)
    {
        @autoreleasepool {
            glfwPollEvents();

            // Skip frames while window is minimised
            int fbw, fbh;
            glfwGetFramebufferSize(m_impl->window, &fbw, &fbh);
            if (fbw == 0 || fbh == 0) continue;

            // Sync Metal layer to framebuffer (handles Retina / resize)
            NSWindow* nsw = glfwGetCocoaWindow(m_impl->window);
            m_impl->metalLayer.drawableSize  = CGSizeMake(fbw, fbh);
            m_impl->metalLayer.contentsScale = [nsw backingScaleFactor];

            id<CAMetalDrawable> drawable = [m_impl->metalLayer nextDrawable];
            if (!drawable) continue;

            m_impl->rpd.colorAttachments[0].texture = drawable.texture;

            id<MTLCommandBuffer>        cmdBuf = [m_impl->commandQueue commandBuffer];
            id<MTLRenderCommandEncoder> enc    =
                [cmdBuf renderCommandEncoderWithDescriptor:m_impl->rpd];
            [enc pushDebugGroup:@"ImGui"];

            // ── ImGui frame ───────────────────────────────────────────────────
            ImGui_ImplMetal_NewFrame(m_impl->rpd);
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            render_ui_frame(m_impl->state, m_impl->engine, io);

            ImGui::Render();
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmdBuf, enc);

            [enc popDebugGroup];
            [enc endEncoding];

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            [cmdBuf presentDrawable:drawable];
            [cmdBuf commit];
        }
    }
}

// ── Vertical splitter drag helper ──────────────────────────────────────────────
// Renders an invisible 4px-wide button that the user can drag to resize a pane.
// Updates *width in-place; constrained to [120, avail_w - 120].

static void draw_vertical_splitter(const char* id, float* width, float avail_w)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    // Thin visible separator line
    const float h     = ImGui::GetContentRegionAvail().y;
    const ImVec2 pos  = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(
        pos, ImVec2(pos.x + 4.0f, pos.y + h),
        IM_COL32(60, 60, 65, 255));

    ImGui::InvisibleButton(id, ImVec2(4.0f, h));
    ImGui::PopStyleVar();

    if (ImGui::IsItemActive()) {
        *width += ImGui::GetIO().MouseDelta.x;
        *width = std::clamp(*width, 120.0f, avail_w - 120.0f);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
}

// ── Three-pane layout + UI composition ────────────────────────────────────────

static void render_ui_frame(AppState& state, PlotEngine& engine, const ImGuiIO& io)
{
    ImGuiViewport* vp = ImGui::GetMainViewport();

    // ── Full-screen host window ────────────────────────────────────────────────
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    constexpr ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking         | ImGuiWindowFlags_NoTitleBar    |
        ImGuiWindowFlags_NoCollapse        | ImGuiWindowFlags_NoResize      |
        ImGuiWindowFlags_NoMove            | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus        | ImGuiWindowFlags_MenuBar       |
        ImGuiWindowFlags_NoScrollbar       | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));
    ImGui::Begin("##HostWindow", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    // ── Menu bar ──────────────────────────────────────────────────────────────
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open…", "Cmd+O")) {
                auto result = show_open_dialog("Open Simulation File", true, {"h5", "hdf5"});
                if (result.confirmed) {
                    static int menu_sim_counter = 0;
                    for (const auto& p : result.paths) {
                        const std::string id = "sim" + std::to_string(menu_sim_counter++);
                        auto sim = std::make_unique<SimulationFile>(id);
                        if (auto res = sim->open(p); res)
                            state.simulations.push_back(std::move(sim));
                        else
                            FASTSCOPE_LOG_ERROR("Open failed: {}", res.error().message);
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export CSV…", nullptr,
                                false, !state.plotted_signals.empty())) {
                auto res = show_save_dialog("Export CSV", {"csv"});
                if (res.confirmed && !res.path.empty()) {
                    auto r = export_csv(state, res.path);
                    if (r)
                        FASTSCOPE_LOG_INFO("CSV exported: {} rows to {}", *r, res.path.string());
                    else
                        FASTSCOPE_LOG_WARN("CSV export failed: {}", r.error().message);
                }
            }
            if (ImGui::MenuItem("Export PNG…")) {
                auto res = show_save_dialog("Export PNG", {"png"});
                if (res.confirmed && !res.path.empty()) {
                    auto r = export_png("FastScope", res.path);
                    if (!r)
                        FASTSCOPE_LOG_WARN("PNG export failed: {}", r.error().message);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Session", "Cmd+S"))
                save_session(state);
            if (ImGui::MenuItem("Load Session\xe2\x80\xa6")) {
                auto result = show_open_dialog("Load Session", false, {"json"});
                if (result.confirmed && !result.paths.empty())
                    load_session(state, result.paths[0]);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Cmd+Q"))
                state.quit_requested = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Files pane",   nullptr, &state.panes.file_pane_visible);
            ImGui::MenuItem("Signals pane", nullptr, &state.panes.signal_pane_visible);
            ImGui::MenuItem("Log window",   nullptr, &state.panes.log_pane_visible);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo",   nullptr, &state.debug.show_imgui_demo);
            ImGui::MenuItem("ImPlot Demo",  nullptr, &state.debug.show_implot_demo);
            ImGui::MenuItem("Metrics",      nullptr, &state.debug.show_metrics);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // ── Three-pane layout ─────────────────────────────────────────────────────
    const float avail_w = ImGui::GetContentRegionAvail().x;
    constexpr float status_bar_h = 22.0f;
    const float avail_h = ImGui::GetContentRegionAvail().y - status_bar_h;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    // ── Left pane: simulation file manager ────────────────────────────────────
    if (state.panes.file_pane_visible) {
        ImGui::BeginChild("##FilesPane",
                          ImVec2(state.panes.file_pane_width, avail_h),
                          ImGuiChildFlags_None);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        render_simulation_list(state);
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::SameLine(0.0f, 0.0f);

        draw_vertical_splitter("##split_files", &state.panes.file_pane_width, avail_w);
        ImGui::SameLine(0.0f, 0.0f);
    }

    // ── Middle pane: signal tree ───────────────────────────────────────────────
    if (state.panes.signal_pane_visible) {
        ImGui::BeginChild("##SignalsPane",
                          ImVec2(state.panes.signal_pane_width, avail_h),
                          ImGuiChildFlags_None);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        render_signal_tree(state);
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::SameLine(0.0f, 0.0f);

        draw_vertical_splitter("##split_signals", &state.panes.signal_pane_width, avail_w);
        ImGui::SameLine(0.0f, 0.0f);
    }

    // ── Right pane: plot area ──────────────────────────────────────────────────
    {
        const float plot_w = ImGui::GetContentRegionAvail().x;
        const float plot_h = avail_h;
        ImGui::BeginChild("##PlotPane", ImVec2(plot_w, plot_h), ImGuiChildFlags_None);

        // Plotted-signal list (simple list at top of plot pane, Phase 6 will expand this)
        if (!state.plotted_signals.empty()) {
            static int  s_editing_idx = -1;
            static char s_edit_buf[512] = {};

            for (int idx = 0; idx < static_cast<int>(state.plotted_signals.size()); ) {
                auto& ps = state.plotted_signals[idx];
                ImGui::PushID(ps.plot_key().c_str());

                // Color swatch
                float col[4];
                ColorManager::to_float4(ps.color_rgba, col);
                if (ImGui::ColorButton("##col", ImVec4(col[0],col[1],col[2],col[3]),
                                       ImGuiColorEditFlags_NoTooltip, ImVec2(14, 14)))
                {}
                ImGui::SameLine();
                // Visibility toggle
                ImGui::Checkbox("##vis", &ps.visible);
                ImGui::SameLine();

                // ── Inline alias editor (Task 1) ──────────────────────────────
                if (s_editing_idx == idx) {
                    // Request focus on the first frame of editing
                    ImGui::SetKeyboardFocusHere();
                    ImGuiInputTextFlags edit_flags =
                        ImGuiInputTextFlags_EnterReturnsTrue |
                        ImGuiInputTextFlags_AutoSelectAll;
                    bool committed = ImGui::InputText("##alias_edit", s_edit_buf,
                                                      sizeof(s_edit_buf), edit_flags);
                    bool lost_focus = !ImGui::IsItemActive() && !ImGui::IsItemFocused();
                    if (committed) {
                        ps.alias = s_edit_buf;
                        s_editing_idx = -1;
                    } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                        s_editing_idx = -1;          // cancel — discard buffer
                    } else if (lost_focus) {
                        ps.alias = s_edit_buf;       // commit on focus loss
                        s_editing_idx = -1;
                    }
                } else {
                    ImGui::TextUnformatted(ps.display_name().c_str());
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        s_editing_idx = idx;
                        // Pre-fill: use existing alias, or h5_path if alias is empty
                        const std::string& src = ps.alias.empty() ? ps.meta.h5_path : ps.alias;
                        std::strncpy(s_edit_buf, src.c_str(), sizeof(s_edit_buf) - 1);
                        s_edit_buf[sizeof(s_edit_buf) - 1] = '\0';
                    }
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", ps.sim_id.c_str());
                ImGui::SameLine();

                // ── Y-axis badge (Task 2) ─────────────────────────────────────
                {
                    static const ImVec4 axis_colors[3] = {
                        ImVec4(0.70f, 0.70f, 0.70f, 1.00f),   // Y1: neutral grey
                        ImVec4(0.50f, 0.70f, 1.00f, 1.00f),   // Y2: blue tint
                        ImVec4(0.50f, 1.00f, 0.60f, 1.00f),   // Y3: green tint
                    };
                    static const char* axis_labels[3] = {"[L]", "[R]", "[Y3]"};
                    int axis = std::clamp(ps.y_axis, 0, 2);
                    ImGui::PushStyleColor(ImGuiCol_Text, axis_colors[axis]);
                    ImGui::TextUnformatted(axis_labels[axis]);
                    ImGui::PopStyleColor();
                }
                ImGui::SameLine();

                // Remove button
                if (ImGui::SmallButton("✕##rm")) {
                    state.colors.release(ps.plot_key());
                    state.plotted_signals.erase(state.plotted_signals.begin() + idx);
                    ImGui::PopID();
                    continue;
                }

                // ── Right-click context menu for Y-axis (Task 2) ──────────────
                if (ImGui::BeginPopupContextItem("##axis_ctx")) {
                    ImGui::TextDisabled("Assign axis");
                    ImGui::Separator();
                    if (ImGui::MenuItem("Left (Y1)",  nullptr, ps.y_axis == 0)) ps.y_axis = 0;
                    if (ImGui::MenuItem("Right (Y2)", nullptr, ps.y_axis == 1)) ps.y_axis = 1;
                    if (ImGui::MenuItem("Y3",         nullptr, ps.y_axis == 2)) ps.y_axis = 2;
                    ImGui::EndPopup();
                }

                ImGui::PopID();
                ++idx;
            }

            // ── Derived signal creator (modal dialog) ──────────────────────────
            if (ImGui::SmallButton("+ Derived"))
                ImGui::OpenPopup("Create Derived Signal");

            if (ImGui::BeginPopupModal("Create Derived Signal", nullptr,
                                        ImGuiWindowFlags_AlwaysAutoResize)) {
                static int  s_op_sel = 0;
                static char s_name_buf[128] = "derived";

                static auto op_reg = fastscope::create_operation_registry();
                const auto& op_keys = op_reg.keys();

                ImGui::Text("Operation:");
                for (int i = 0; i < static_cast<int>(op_keys.size()); ++i) {
                    ImGui::SameLine();
                    if (ImGui::RadioButton(op_keys[i].c_str(), &s_op_sel, i)) {}
                }

                ImGui::Text("Inputs (select 1-N signals from the plot):");
                static std::vector<uint8_t> s_input_sel;
                if (s_input_sel.size() != state.plotted_signals.size())
                    s_input_sel.assign(state.plotted_signals.size(), 0);

                for (int i = 0; i < static_cast<int>(state.plotted_signals.size()); ++i) {
                    const auto& ps = state.plotted_signals[i];
                    bool checked = s_input_sel[i] != 0;
                    if (ImGui::Checkbox(ps.display_name().c_str(), &checked))
                        s_input_sel[i] = checked ? 1 : 0;
                }

                ImGui::InputText("Name", s_name_buf, sizeof(s_name_buf));
                ImGui::Separator();

                if (ImGui::Button("Create")) {
                    std::vector<std::shared_ptr<fastscope::SignalBuffer>> inputs;
                    for (int i = 0; i < static_cast<int>(state.plotted_signals.size()); ++i) {
                        if (s_input_sel[i] && state.plotted_signals[i].buffer)
                            inputs.push_back(state.plotted_signals[i].buffer);
                    }

                    if (!inputs.empty() && s_op_sel < static_cast<int>(op_keys.size())) {
                        auto op = op_reg.create(op_keys[s_op_sel]);
                        fastscope::DerivedSignal ds;
                        ds.id = std::string("derived_") +
                                std::to_string(state.plotted_signals.size());
                        ds.display_name = s_name_buf;
                        ds.operation    = std::move(op);
                        ds.inputs       = std::move(inputs);

                        auto result = ds.compute();
                        if (result) {
                            fastscope::PlottedSignal ps;
                            ps.sim_id       = "derived";
                            ps.meta.h5_path = ds.display_name;
                            ps.meta.name    = ds.display_name;
                            ps.buffer       = *result;
                            ps.alias        = ds.display_name;
                            ps.color_rgba   = state.colors.assign(ds.id);
                            state.plotted_signals.push_back(std::move(ps));
                        } else {
                            FASTSCOPE_LOG_WARN("Derived signal compute failed: {}",
                                         result.error().message);
                        }
                    }

                    s_input_sel.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    s_input_sel.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();
        }

        // ── Plot type selector ────────────────────────────────────────────────
        ImGui::TextDisabled("Plot:");
        ImGui::SameLine();
        for (const auto& type_id : engine.available_types()) {
            const bool is_active = (type_id == engine.current_type_id());
            if (is_active)
                ImGui::PushStyleColor(ImGuiCol_Button,
                                      ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            if (ImGui::SmallButton(type_id.c_str()))
                engine.switch_to(type_id, state);
            if (is_active)
                ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        ImGui::Dummy(ImVec2(8.0f, 0.0f));
        ImGui::SameLine();

        // ── Tool toolbar ──────────────────────────────────────────────────────
        ImGui::TextDisabled("Tools:");
        ImGui::SameLine();
        for (const auto& tool_id : state.tool_manager.available_tools()) {
            const bool active = state.tool_manager.is_active(tool_id);
            if (active) ImGui::PushStyleColor(ImGuiCol_Button,
                            ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            std::string btn_label = tool_id + "##tool";
            if (ImGui::SmallButton(btn_label.c_str()))
                state.tool_manager.activate(tool_id);
            if (active) ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        ImGui::Dummy(ImVec2(8.0f, 0.0f));
        ImGui::SameLine();

        // ── Fit toolbar ───────────────────────────────────────────────────────
        if (ImGui::SmallButton("Fit All"))  engine.fit_to_data();
        ImGui::SameLine();
        if (ImGui::SmallButton("Fit X"))    engine.fit_x_only();
        ImGui::SameLine();
        if (ImGui::SmallButton("Fit Y"))    engine.fit_y_only();
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(0, 0));  // flush sameline

        // Plot fills remaining height
        const float remaining_h = ImGui::GetContentRegionAvail().y;
        engine.render(state, -1.0f, remaining_h > 20.0f ? remaining_h : -1.0f);

        ImGui::EndChild();
    }

    // ItemSpacing is still 0 here — do NOT pop until after the status bar so
    // no gap is inserted between the panes and the bar.

    // ── Status bar ────────────────────────────────────────────────────────────
    {
        const SystemStats sys    = query_system_stats();
        const float       fps    = io.Framerate;
        const std::size_t n_sims = state.simulations.size();
        const std::size_t n_sig  = state.plotted_signals.size();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.13f, 1.0f));
        ImGui::BeginChild("##StatusBar", ImVec2(-1.0f, status_bar_h), ImGuiChildFlags_None);

        ImGui::PopStyleVar();  // ItemSpacing (restore here, inside the child)

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 0.0f));
        ImGui::SetCursorPosY(4.0f);
        ImGui::SetCursorPosX(8.0f);

        // FPS
        ImGui::TextDisabled("%.1f fps", fps);
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0.0f, 12.0f);

        // Simulations loaded
        ImGui::Text("%zu file%s", n_sims, n_sims == 1 ? "" : "s");
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0.0f, 12.0f);

        // Signals plotted
        ImGui::Text("%zu signal%s plotted", n_sig, n_sig == 1 ? "" : "s");
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0.0f, 12.0f);

        // CPU %
        ImGui::TextDisabled("CPU %.1f%%", sys.cpu_pct);
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0.0f, 12.0f);

        // Memory RSS
        ImGui::TextDisabled("Mem %.0f MB", sys.rss_mb);

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::End();

    // ── Floating windows ──────────────────────────────────────────────────────
    if (state.panes.log_pane_visible)
        render_log_window(&state.panes.log_pane_visible);
    if (state.debug.show_imgui_demo)  ImGui::ShowDemoWindow(&state.debug.show_imgui_demo);
    if (state.debug.show_implot_demo) ImPlot::ShowDemoWindow(&state.debug.show_implot_demo);
    if (state.debug.show_metrics)     ImGui::ShowMetricsWindow(&state.debug.show_metrics);
}

// ── Shutdown ───────────────────────────────────────────────────────────────────

void Application::shutdown()
{
    if (!m_impl->initialized) return;

    FASTSCOPE_LOG_INFO("Shutting down");

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_impl->window);
    m_impl->window = nullptr;
    glfwTerminate();

    m_impl->initialized = false;
    fastscope::log::shutdown();
}

bool Application::is_initialized() const noexcept
{
    return m_impl && m_impl->initialized;
}

} // namespace fastscope
