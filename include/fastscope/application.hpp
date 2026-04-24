#pragma once
/// @file application.hpp
/// @brief Main application class: GLFW + Metal + ImGui lifecycle. PIMPL pattern.
/// @ingroup app_layer

#include <memory>
#include <string_view>

namespace fastscope {

/// @brief Main application class: GLFW window, Metal device, and ImGui lifecycle.
/// @details Owns all ObjC/Metal types behind a PIMPL so this header is plain C++.
///          Typical usage: construct, call init(), call run(), destructor cleans up.
class Application {
public:
    Application();
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    /// @brief Window and ImGui configuration for Application::init(Config).
    struct Config {
        int              width            = 1600;
        int              height           = 900;
        std::string_view title            = "FastScope";
        bool             enable_docking   = true;
        bool             enable_viewports = true;  ///< Multi-viewport / detachable windows.
    };

    /// Set up window, Metal device, ImGui with default settings.
    [[nodiscard]] bool init();

    /// Set up window, Metal device, ImGui with custom settings.
    [[nodiscard]] bool init(Config cfg);

    /// Run the main loop until the window is closed.
    void run();

    /// Release all resources. Called automatically by destructor.
    void shutdown();

    [[nodiscard]] bool is_initialized() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace fastscope
