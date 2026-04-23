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
#include "gnc_viz/version.hpp"

#include <cstdio>
#include <string>

namespace gnc_viz {

// ── forward declaration ────────────────────────────────────────────────────────
static void render_ui_frame(bool& show_demo, bool& show_implot, const ImGuiIO& io);

// ── PIMPL — holds all ObjC / Metal / GLFW state ───────────────────────────────

struct Application::Impl {
    GLFWwindow*              window       = nullptr;
    id<MTLDevice>            device;
    id<MTLCommandQueue>      commandQueue;
    CAMetalLayer*            metalLayer;
    MTLRenderPassDescriptor* rpd;

    bool initialized = false;
    Config config;
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

    // ── GLFW ──────────────────────────────────────────────────────────────────
    glfwSetErrorCallback([](int error, const char* description) {
        std::printf("[gnc_viz] GLFW Error %d: %s\n", error, description);
    });

    if (!glfwInit()) {
        std::printf("[gnc_viz] Failed to initialise GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API,              GLFW_NO_API);   // Metal, not GL
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE,               GLFW_TRUE);

    const std::string title{cfg.title};
    m_impl->window = glfwCreateWindow(cfg.width, cfg.height, title.c_str(),
                                      nullptr, nullptr);
    if (!m_impl->window) {
        std::printf("[gnc_viz] Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    // ── Metal ─────────────────────────────────────────────────────────────────
    m_impl->device = MTLCreateSystemDefaultDevice();
    if (!m_impl->device) {
        std::printf("[gnc_viz] Metal is not supported on this device\n");
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
        ImGuiStyle& style            = ImGui::GetStyle();
        style.WindowRounding         = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOther(m_impl->window, /*install_callbacks=*/true);
    ImGui_ImplMetal_Init(m_impl->device);

    // ── Render pass descriptor (reused each frame; texture swapped per frame) ─
    m_impl->rpd = [MTLRenderPassDescriptor new];
    m_impl->rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
    m_impl->rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
    m_impl->rpd.colorAttachments[0].clearColor  = MTLClearColorMake(0.10, 0.10, 0.12, 1.0);

    m_impl->initialized = true;
    return true;
}

// ── Main loop ──────────────────────────────────────────────────────────────────

void Application::run()
{
    if (!m_impl->initialized) return;

    ImGuiIO& io = ImGui::GetIO();

    bool show_demo   = false;
    bool show_implot = false;

    while (!glfwWindowShouldClose(m_impl->window))
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

            render_ui_frame(show_demo, show_implot, io);

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

// ── UI composition (extracted so run() stays readable) ────────────────────────

static void render_ui_frame(bool& show_demo, bool& show_implot,
                             const ImGuiIO& io)
{
    // Full-screen dockspace
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        constexpr ImGuiWindowFlags hf =
            ImGuiWindowFlags_NoDocking         | ImGuiWindowFlags_NoTitleBar   |
            ImGuiWindowFlags_NoCollapse        | ImGuiWindowFlags_NoResize     |
            ImGuiWindowFlags_NoMove            | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus        | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_MenuBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0, 0));
        ImGui::Begin("##DockHost", nullptr, hf);
        ImGui::PopStyleVar(3);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("ImGui Demo",  nullptr, &show_demo);
                ImGui::MenuItem("ImPlot Demo", nullptr, &show_implot);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0),
                         ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();
    }

    if (show_demo)   ImGui::ShowDemoWindow(&show_demo);
    if (show_implot) ImPlot::ShowDemoWindow(&show_implot);

    // Welcome panel (placeholder until proper panels are wired in)
    ImGui::Begin("GNC Viz");
    ImGui::TextUnformatted("GNC Simulation Data Visualizer");
    ImGui::Text("v%s", gnc_viz::version());
    ImGui::Separator();
    ImGui::Text("%.1f FPS  (%.3f ms/frame)", io.Framerate, 1000.f / io.Framerate);
    ImGui::Separator();
    ImGui::TextDisabled("Load a simulation file to begin.");
    ImGui::End();
}

// ── Shutdown ───────────────────────────────────────────────────────────────────

void Application::shutdown()
{
    if (!m_impl->initialized) return;

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_impl->window);
    m_impl->window = nullptr;
    glfwTerminate();

    m_impl->initialized = false;
}

bool Application::is_initialized() const noexcept
{
    return m_impl && m_impl->initialized;
}

} // namespace gnc_viz
