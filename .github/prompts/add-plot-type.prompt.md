# Prompt: Add a New Plot Type to FastScope

Use this prompt when you need to implement a new way to visualize data (e.g. a 2D X-Y trajectory plot,
a histogram, a waterfall, a phase-space plot, etc.).

---

## Context

FastScope (`/Users/thomasbarbier/Workspace/curly-giggle`) is a C++23 + Dear ImGui + Metal macOS app
that visualizes HDF5 time-series files. Plot types implement `IPlotType`
(`include/fastscope/interfaces.hpp`) and are registered in `PlotEngine`
(`src/app/plot_engine.cpp`). Data lives in `AppState::plotted_signals` (vector of `PlottedSignal`).

Key files to read before starting:
- `include/fastscope/interfaces.hpp` — IPlotType interface definition
- `include/fastscope/app_state.hpp` — AppState + PlottedSignal structures
- `include/fastscope/plotted_signal.hpp` — PlottedSignal, float caches, display_name()
- `src/app/timeseries_plot.cpp` — Reference implementation with decimation + culling
- `src/app/plot_engine.cpp` — Registration point

---

## Step 1 — Create the header

File: `include/fastscope/my_plot.hpp`

```cpp
#pragma once
/// @file my_plot.hpp
/// @brief [One-line description of what this plot type shows].
#include "fastscope/interfaces.hpp"

namespace fastscope {

/// @brief [Full description].
/// @details Renders [what] from AppState::plotted_signals using ImPlot.
class MyPlot final : public IPlotType {
public:
    /// @brief Human-readable name shown in the plot-type selector.
    [[nodiscard]] std::string_view name() const noexcept override { return "My Plot"; }

    /// @brief Unique registry key (lowercase, no spaces).
    [[nodiscard]] std::string_view id() const noexcept override { return "myplot"; }

    void render(AppState& state, float width, float height) override;
    void on_activate(AppState& state) override;
    void on_deactivate() override;

private:
    bool m_fit_on_next_frame = true;
    // Add per-instance state here (not in AppState — keep it local to the plot type)
};

} // namespace fastscope
```

---

## Step 2 — Create the source file

File: `src/app/my_plot.cpp`

```cpp
#include "fastscope/my_plot.hpp"
#include "fastscope/app_state.hpp"
#include "fastscope/plotted_signal.hpp"
#include <implot.h>

namespace fastscope {

void MyPlot::on_activate(AppState& /*state*/) {
    m_fit_on_next_frame = true;
}

void MyPlot::on_deactivate() {
    // Clean up any ImPlot state if needed
}

void MyPlot::render(AppState& state, float width, float height) {
    ImPlotFlags flags = ImPlotFlags_NoFrame;

    if (m_fit_on_next_frame) {
        ImPlot::SetNextAxesToFit();
        m_fit_on_next_frame = false;
    }

    if (ImPlot::BeginPlot("##myplot", {width, height}, flags)) {
        ImPlot::SetupAxes("X Label", "Y Label");

        for (auto& sig : state.plotted_signals) {
            if (!sig.visible || sig.component_cache.empty()) continue;

            // Use sig.time_cache_f and sig.component_cache[i] (pre-converted floats)
            // NEVER allocate a temp vector here — use the cache directly
            const auto& xs = sig.time_cache_f;
            const auto& ys = sig.component_cache[0];
            const int   n  = static_cast<int>(std::min(xs.size(), ys.size()));

            ImPlot::SetNextLineStyle(sig.color);
            ImPlot::PlotLine(sig.display_name().c_str(),
                             xs.data(), ys.data(), n);
        }

        ImPlot::EndPlot();
    }
}

} // namespace fastscope
```

**Performance rules:**
- Never allocate a `std::vector` inside `render()`.
- Use `sig.time_cache_f` and `sig.component_cache` — they are pre-converted `float` vectors.
- For large datasets implement stride decimation (see `timeseries_plot.cpp` for the pattern).

---

## Step 3 — Register the plot type

File: `src/app/plot_engine.cpp`

```cpp
#include "fastscope/my_plot.hpp"   // ← add this include at the top

PlotEngine::PlotEngine() {
    m_registry.register_type<TimeSeriesPlot>("timeseries");
    m_registry.register_type<MyPlot>("myplot");   // ← add this line
    // ...
}
```

---

## Step 4 — Add to CMakeLists.txt

File: `src/app/CMakeLists.txt`

```cmake
add_executable(fastscope
    # ... existing sources ...
    my_plot.cpp    # ← add this line
)
```

---

## Step 5 — (Optional) Expose in plot-type selector UI

If the plot type should be user-selectable, it will automatically appear in any UI that calls
`m_plot_engine.registry().keys()` — no additional UI changes are needed unless you want custom
configuration panels.

---

## Acceptance Criteria

Before committing:
1. `cmake --build build -j$(sysctl -n hw.ncpu)` — zero errors and zero warnings
2. `./build/bin/fastscope_tests` — all tests pass (no regressions)
3. Launch app → switch to "My Plot" in the plot-type selector → signals render correctly
4. FPS stays at 60 with ≥4 signals plotted (use Instruments if unsure)
5. `m_fit_on_next_frame` resets zoom when the plot type is activated
6. Add `///` Doxygen comments to all public members in the header
