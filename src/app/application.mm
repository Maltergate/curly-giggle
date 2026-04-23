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

#include "gnc_viz/application.hpp"
#include "gnc_viz/app_state.hpp"
#include "gnc_viz/log.hpp"
#include "gnc_viz/version.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

namespace gnc_viz {

// ── forward declarations ───────────────────────────────────────────────────────
static void render_ui_frame(AppState& state, const ImGuiIO& io);
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
    gnc_viz::log::init({
        .file_path = "gnc_viz.log",
        .console   = true,
        .file      = false,  // file log opt-in: avoid polluting cwd during dev
        .level     = spdlog::level::debug,
    });
    GNC_LOG_INFO("GNC Viz {} starting up", gnc_viz::version());

    // ── GLFW ──────────────────────────────────────────────────────────────────
    glfwSetErrorCallback([](int error, const char* description) {
        GNC_LOG_ERROR("GLFW error {}: {}", error, description);
    });

    if (!glfwInit()) {
        GNC_LOG_ERROR("Failed to initialise GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API,               GLFW_NO_API);   // Metal, not GL
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE,                GLFW_TRUE);

    const std::string title{cfg.title};
    m_impl->window = glfwCreateWindow(cfg.width, cfg.height, title.c_str(),
                                      nullptr, nullptr);
    if (!m_impl->window) {
        GNC_LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    // ── Metal ─────────────────────────────────────────────────────────────────
    m_impl->device = MTLCreateSystemDefaultDevice();
    if (!m_impl->device) {
        GNC_LOG_ERROR("Metal is not supported on this device");
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
    GNC_LOG_INFO("Application initialised");
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

            render_ui_frame(m_impl->state, io);

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

static void render_ui_frame(AppState& state, const ImGuiIO& io)
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
        ImGuiWindowFlags_NoNavFocus        | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));
    ImGui::Begin("##HostWindow", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    // ── Menu bar ──────────────────────────────────────────────────────────────
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Quit", "Cmd+Q"))
                state.quit_requested = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Files pane",   nullptr, &state.panes.file_pane_visible);
            ImGui::MenuItem("Signals pane", nullptr, &state.panes.signal_pane_visible);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo",   nullptr, &state.debug.show_imgui_demo);
            ImGui::MenuItem("ImPlot Demo",  nullptr, &state.debug.show_implot_demo);
            ImGui::MenuItem("Metrics",      nullptr, &state.debug.show_metrics);
            ImGui::EndMenu();
        }
        ImGui::Text("%.0f fps", io.Framerate);
        ImGui::EndMenuBar();
    }

    // ── Three-pane layout ─────────────────────────────────────────────────────
    const float avail_w = ImGui::GetContentRegionAvail().x;
    const float avail_h = ImGui::GetContentRegionAvail().y;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    // ── Left pane: file loader ─────────────────────────────────────────────────
    if (state.panes.file_pane_visible) {
        ImGui::BeginChild("##FilesPane",
                          ImVec2(state.panes.file_pane_width, avail_h),
                          ImGuiChildFlags_None);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        ImGui::TextUnformatted("Simulations");
        ImGui::Separator();
        ImGui::TextDisabled("Drop .h5 files here\nor use File → Open");
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::SameLine(0.0f, 0.0f);

        draw_vertical_splitter("##split_files", &state.panes.file_pane_width, avail_w);
        ImGui::SameLine(0.0f, 0.0f);
    }

    // ── Middle pane: signal manager ────────────────────────────────────────────
    if (state.panes.signal_pane_visible) {
        ImGui::BeginChild("##SignalsPane",
                          ImVec2(state.panes.signal_pane_width, avail_h),
                          ImGuiChildFlags_None);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        ImGui::TextUnformatted("Signals");
        ImGui::Separator();
        ImGui::TextDisabled("Select a simulation to\nbrowse its signals.");
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::SameLine(0.0f, 0.0f);

        draw_vertical_splitter("##split_signals", &state.panes.signal_pane_width, avail_w);
        ImGui::SameLine(0.0f, 0.0f);
    }

    // ── Right pane: plot area (fills remaining width) ──────────────────────────
    ImGui::BeginChild("##PlotPane", ImVec2(-1.0f, avail_h), ImGuiChildFlags_None);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::TextUnformatted("Plot Area");
    ImGui::Separator();
    ImGui::TextDisabled("Add signals from the Signals pane to plot them here.");
    ImGui::Text("GNC Viz %s  |  Metal + Dear ImGui", gnc_viz::version());
    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::PopStyleVar();  // ItemSpacing
    ImGui::End();

    // ── Debug windows ─────────────────────────────────────────────────────────
    if (state.debug.show_imgui_demo)  ImGui::ShowDemoWindow(&state.debug.show_imgui_demo);
    if (state.debug.show_implot_demo) ImPlot::ShowDemoWindow(&state.debug.show_implot_demo);
    if (state.debug.show_metrics)     ImGui::ShowMetricsWindow(&state.debug.show_metrics);
}

// ── Shutdown ───────────────────────────────────────────────────────────────────

void Application::shutdown()
{
    if (!m_impl->initialized) return;

    GNC_LOG_INFO("Shutting down");

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_impl->window);
    m_impl->window = nullptr;
    glfwTerminate();

    m_impl->initialized = false;
    gnc_viz::log::shutdown();
}

bool Application::is_initialized() const noexcept
{
    return m_impl && m_impl->initialized;
}

} // namespace gnc_viz
