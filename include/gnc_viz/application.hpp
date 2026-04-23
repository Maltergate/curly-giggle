#pragma once

#include <memory>
#include <string_view>

namespace gnc_viz {

// ── Application ────────────────────────────────────────────────────────────────
//
// Owns the GLFW window, Metal device, and ImGui context.
// All ObjC / Metal types are hidden behind a PIMPL so this header is plain C++
// and can be included from non-.mm translation units.
//
// Typical usage:
//   int main() {
//       gnc_viz::Application app;
//       if (!app.init()) return 1;
//       app.run();
//   }

class Application {
public:
    Application();
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    struct Config {
        int              width            = 1600;
        int              height           = 900;
        std::string_view title            = "GNC Viz";
        bool             enable_docking   = true;
        bool             enable_viewports = true;  // multi-viewport / detachable windows
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

} // namespace gnc_viz
