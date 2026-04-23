// main.mm — GNC Viz application entry point
// Platform: macOS | Renderer: Metal | Window: GLFW | GUI: Dear ImGui (docking)

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>

// GLFW with Cocoa native access (needed for glfwGetCocoaWindow)
// GLFW_EXPOSE_NATIVE_COCOA is defined via CMake compile definitions
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include "implot.h"

#include "gnc_viz/version.hpp"

#include <cstdio>
#include <format>

// ── GLFW error callback ────────────────────────────────────────────────────────

static void glfw_error_callback(int error, const char* description)
{
    std::printf("GLFW Error %d: %s\n", error, description);
}

// ── Entry point ────────────────────────────────────────────────────────────────

int main()
{
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::printf("Failed to initialise GLFW\n");
        return 1;
    }

    // No OpenGL context — Metal handles rendering
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    const auto title = std::format("GNC Viz  v{}", gnc_viz::VERSION_STRING);
    GLFWwindow* window = glfwCreateWindow(1600, 900, title.c_str(), nullptr, nullptr);
    if (!window) {
        std::printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    // ── Metal device + command queue ───────────────────────────────────────────

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        std::printf("Metal is not supported on this device\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    id<MTLCommandQueue> commandQueue = [device newCommandQueue];

    // ── Attach CAMetalLayer to the GLFW window's NSView ───────────────────────

    NSWindow* nswindow     = glfwGetCocoaWindow(window);
    NSView*   contentView  = [nswindow contentView];
    [contentView setWantsLayer:YES];

    CAMetalLayer* metalLayer       = [CAMetalLayer layer];
    metalLayer.device              = device;
    metalLayer.pixelFormat         = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly     = YES;
    metalLayer.contentsScale       = [nswindow backingScaleFactor];
    [contentView setLayer:metalLayer];

    // ── ImGui context ──────────────────────────────────────────────────────────

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // detachable windows

    ImGui::StyleColorsDark();

    // When viewports are enabled, make secondary windows blend with the OS
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding               = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w  = 1.0f;
    }

    ImGui_ImplGlfw_InitForOther(window, /*install_callbacks=*/true);
    ImGui_ImplMetal_Init(device);

    // ── Render pass descriptor (reused every frame, texture swapped) ───────────

    MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor new];
    rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
    rpd.colorAttachments[0].clearColor  = MTLClearColorMake(0.10, 0.10, 0.12, 1.0);

    // ── Render loop ────────────────────────────────────────────────────────────

    bool show_demo   = false;
    bool show_implot = false;

    while (!glfwWindowShouldClose(window))
    {
        @autoreleasepool {
            glfwPollEvents();

            // Handle window resize / Retina scale change
            int fbw, fbh;
            glfwGetFramebufferSize(window, &fbw, &fbh);
            if (fbw == 0 || fbh == 0) continue;

            metalLayer.drawableSize   = CGSizeMake(fbw, fbh);
            metalLayer.contentsScale  = [nswindow backingScaleFactor];

            id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
            if (!drawable) continue;

            rpd.colorAttachments[0].texture = drawable.texture;

            id<MTLCommandBuffer> cmdBuf = [commandQueue commandBuffer];
            id<MTLRenderCommandEncoder> enc =
                [cmdBuf renderCommandEncoderWithDescriptor:rpd];
            [enc pushDebugGroup:@"ImGui"];

            // ── New ImGui frame ────────────────────────────────────────────────
            ImGui_ImplMetal_NewFrame(rpd);
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // ── Full-screen dockspace ──────────────────────────────────────────
            {
                ImGuiViewport* vp = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(vp->WorkPos);
                ImGui::SetNextWindowSize(vp->WorkSize);
                ImGui::SetNextWindowViewport(vp->ID);

                ImGuiWindowFlags host_flags =
                    ImGuiWindowFlags_NoDocking   | ImGuiWindowFlags_NoTitleBar   |
                    ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_NoResize     |
                    ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_NoNavFocus  | ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_MenuBar;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0, 0));
                ImGui::Begin("##DockHost", nullptr, host_flags);
                ImGui::PopStyleVar(3);

                // Menu bar
                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("View")) {
                        ImGui::MenuItem("ImGui Demo",   nullptr, &show_demo);
                        ImGui::MenuItem("ImPlot Demo",  nullptr, &show_implot);
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                ImGuiID dock_id = ImGui::GetID("MainDockSpace");
                ImGui::DockSpace(dock_id, ImVec2(0, 0),
                                 ImGuiDockNodeFlags_PassthruCentralNode);
                ImGui::End();
            }

            // ── Optional demo windows ──────────────────────────────────────────
            if (show_demo)   ImGui::ShowDemoWindow(&show_demo);
            if (show_implot) ImPlot::ShowDemoWindow(&show_implot);

            // ── Persistent info window ─────────────────────────────────────────
            ImGui::Begin("GNC Viz");
            ImGui::TextUnformatted("GNC Simulation Data Visualizer");
            ImGui::Text("v%s", gnc_viz::version());
            ImGui::Separator();
            ImGui::Text("%.1f FPS  (%.3f ms/frame)", io.Framerate, 1000.f / io.Framerate);
            ImGui::Separator();
            ImGui::TextDisabled("Load a simulation file to begin.");
            ImGui::End();

            // ── Render ─────────────────────────────────────────────────────────
            ImGui::Render();
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmdBuf, enc);

            [enc popDebugGroup];
            [enc endEncoding];

            // Multi-viewport: render secondary (detached) windows
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            [cmdBuf presentDrawable:drawable];
            [cmdBuf commit];
        }
    }

    // ── Cleanup ────────────────────────────────────────────────────────────────

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
