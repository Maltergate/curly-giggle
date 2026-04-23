@page developer_manual Developer Manual
@brief Guide for contributors and developers extending GNC Viz

# Developer Manual

This document is for engineers who want to build GNC Viz from source, understand its
architecture, or add new features (plot types, signal operations, visualization tools).

---

## Table of Contents

1. [Build System](#build-system)
2. [Code Style](#code-style)
3. [Architecture Overview](#architecture-overview)
4. [How to Add a New Plot Type](#new-plot-type)
5. [How to Add a New Signal Operation](#new-signal-operation)
6. [How to Add a New Visualization Tool](#new-viz-tool)
7. [The Registry<T> Template](#registry)
8. [SignalBuffer Memory Model](#signal-buffer-memory)
9. [Testing (Catch2)](#testing)
10. [Session JSON Schema](#session-schema)
11. [Error Handling (gnc::Result<T>)](#error-handling)

---

## 1. Build System {#build-system}

GNC Viz uses **CMake 3.28+** with `FetchContent` for third-party dependencies.

### Dependencies fetched automatically

| Library | Purpose |
|---------|---------|
| Dear ImGui | Immediate-mode UI |
| ImPlot | Plotting widget |
| HDF5 (system) | HDF5 file I/O — installed via `brew install hdf5` |
| spdlog | Structured logging |
| Catch2 | Unit tests |
| nlohmann/json | Session & config serialization |

### Configure and build

```bash
# Debug build (with assertions)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# Release build
cmake -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j

# Generate documentation (requires Doxygen)
cmake --build build --target docs
```

### CMake targets

| Target | Description |
|--------|-------------|
| `gnc_viz` | Main application binary |
| `gnc_viz_lib` | Static library (all non-main code) |
| `gnc_viz_tests` | Catch2 test runner |
| `docs` | Doxygen HTML documentation |

---

## 2. Code Style {#code-style}

GNC Viz targets **C++23**.  The following rules are enforced by code review:

### No raw owning pointers

All owned heap objects use `std::unique_ptr` or `std::shared_ptr`.
Raw pointers are only used as *non-owning* references (equivalent to a reference,
but nullable).  Never call `new` or `delete` directly.

```cpp
// Correct
auto sim = std::make_unique<SimulationFile>("sim0");
AppState state;
state.simulations.push_back(std::move(sim));

// Wrong — do NOT do this
SimulationFile* sim = new SimulationFile("sim0");
```

### Use gnc::Result<T> for fallible operations

Functions that can fail return `gnc::Result<T>` (alias for `std::expected<T, gnc::Error>`).
They must never throw exceptions (except in tests).

```cpp
gnc::Result<std::shared_ptr<SignalBuffer>> load_signal(const SignalMetadata& meta);
```

Use `GNC_TRY(expr)` to propagate errors without boilerplate:

```cpp
gnc::Result<Foo> my_func() {
    auto x = GNC_TRY(step_one());   // returns early with error if step_one() failed
    auto y = GNC_TRY(step_two(x));
    return Foo{x, y};
}
```

### No globals, no singletons

All state lives in `AppState`.  Subsystems receive a `(const) AppState&` parameter.
The logger is the only exception (accessed via `gnc_viz::log::get()`).

### Naming conventions

| Symbol | Convention | Example |
|--------|-----------|---------|
| Types | `PascalCase` | `SignalBuffer`, `PlotEngine` |
| Functions / methods | `snake_case` | `load_signal()`, `fit_to_data()` |
| Member variables | `m_snake_case` | `m_sim_id`, `m_cache` |
| Constants | `kPascalCase` or `ALL_CAPS` | `kCollapsedWidth`, `max_axes` |
| Namespaces | `snake_case` | `gnc_viz`, `gnc` |

---

## 3. Architecture Overview {#architecture-overview}

```
┌────────────────────────────────────────────────────────────────────┐
│ Application (application.hpp / application.mm)                     │
│  GLFW + Metal + ImGui main loop                                    │
│  Owns: AppState, PlotEngine                                        │
├──────────────────────┬─────────────────────────────────────────────┤
│   Left Pane          │   Centre Pane         │  Right Pane          │
│   SimulationListUI   │   SignalTreeWidget    │  PlotEngine          │
│   (simulation_       │   (signal_tree_       │  ┌──────────────┐   │
│    list_ui.hpp)      │    widget.hpp)        │  │ IPlotType    │   │
├──────────────────────┴───────────────────────┤  │ (active)     │   │
│                  AppState                    │  └──────────────┘   │
│  simulations[]     → SimulationFile          │  ToolManager         │
│  plotted_signals[] → PlottedSignal           │  AxisManager         │
│  axis_manager      → AxisManager             │  ColorManager        │
│  tool_manager      → ToolManager             │                      │
│  colors            → ColorManager            │                      │
│  panes             → PaneState               │                      │
└──────────────────────────────────────────────┴──────────────────────┘
```

### Ownership model

```
Application
  └─ AppState (value member, not heap-allocated)
       ├─ simulations: vector<unique_ptr<SimulationFile>>
       │     └─ HDF5Reader (PIMPL, owns libhdf5 handle)
       │     └─ cache: map<string, weak_ptr<SignalBuffer>>
       ├─ plotted_signals: vector<PlottedSignal>
       │     └─ buffer: shared_ptr<SignalBuffer>   ← shared with cache
       ├─ axis_manager: AxisManager (value)
       ├─ tool_manager: ToolManager (value)
       │     └─ active: unique_ptr<IVisualizationTool>
       └─ colors: ColorManager (value)
```

### Data flow: file open → plot render

```
User drops .h5 file
  │
  ▼
SimulationFile::open()
  │  HDF5Reader::enumerate_signals() → vector<SignalMetadata>
  │  HDF5Reader::suggest_time_axes() → vector<string>
  │
  ▼
AppState::simulations.push_back(sim)
  │
  ▼
User clicks [+] on a signal
  │
  ▼
PlottedSignal created (buffer = nullptr)
AppState::plotted_signals.push_back(ps)
ColorManager::assign(plot_key)
  │
  ▼
PlotEngine::render() called each frame
  │  TimeSeriesPlot::render(state, w, h)
  │    for each PlottedSignal where buffer == nullptr:
  │      SimulationFile::load_signal(meta) → shared_ptr<SignalBuffer>
  │      ps.buffer = buf;  build float caches
  │    ImPlot::PlotLine(time_cache_f, component_cache[k], N)
  │
  ▼
Screen
```

### Thread model

GNC Viz is **single-threaded**.  All UI, I/O, and rendering happen on the main thread.
HDF5 reads happen during the render loop (on-demand, first frame).  For large files this
can cause a single-frame hitch; background loading is planned but not yet implemented.

---

## 4. How to Add a New Plot Type {#new-plot-type}

A plot type renders the signals in `AppState::plotted_signals` in some way.  It inherits
from `IPlotType` (see `include/gnc_viz/interfaces.hpp`).

### Step-by-step

**Step 1 — Create the header** `include/gnc_viz/my_plot.hpp`:

```cpp
#pragma once
#include "gnc_viz/interfaces.hpp"

namespace gnc_viz {

class MyPlot : public IPlotType {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "My Plot"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "myplot"; }

    void render(AppState& state, float width, float height) override;
    void on_activate(AppState& state) override;
    void on_deactivate() override;

private:
    bool m_fit_on_next = true;
};

} // namespace gnc_viz
```

**Step 2 — Create the source file** `src/lib/my_plot.cpp`:

```cpp
#include "gnc_viz/my_plot.hpp"
#include "gnc_viz/app_state.hpp"
#include <implot.h>

namespace gnc_viz {

void MyPlot::on_activate(AppState&) { m_fit_on_next = true; }
void MyPlot::on_deactivate() {}

void MyPlot::render(AppState& state, float width, float height) {
    if (ImPlot::BeginPlot("##myplot", {width, height})) {
        // Load buffers, render series, etc.
        ImPlot::EndPlot();
    }
}

} // namespace gnc_viz
```

**Step 3 — Register in PlotEngine constructor** (`src/lib/plot_engine.cpp`):

```cpp
#include "gnc_viz/my_plot.hpp"

PlotEngine::PlotEngine() {
    m_registry.register_type<TimeSeriesPlot>("timeseries");
    m_registry.register_type<Trajectory2DPlot>("trajectory2d");
    m_registry.register_type<GroundTrackPlot>("groundtrack");
    m_registry.register_type<MyPlot>("myplot");   // ← add this line
    switch_to("timeseries", /* dummy state */ ...);
}
```

**Step 4 — Add to CMakeLists.txt**:

Add `src/lib/my_plot.cpp` to the `gnc_viz_lib` target sources.

### Interface contract

| Method | Called when | Must do |
|--------|-------------|---------|
| `name()` | UI selector list | Return human-readable name |
| `id()` | Registry key | Return unique lowercase slug |
| `render()` | Every frame | Call `ImPlot::BeginPlot()` / `EndPlot()`, draw series |
| `on_activate()` | Plot type selected | Reset zoom flags, clear per-type state |
| `on_deactivate()` | Another type selected | Clean up ImPlot state if needed |

---

## 5. How to Add a New Signal Operation {#new-signal-operation}

Signal operations inherit from `ISignalOperation` and transform N input `SignalBuffer`s
into one output `SignalBuffer`.

**Step 1 — Add the class** to `include/gnc_viz/signal_ops.hpp`:

```cpp
class MyOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "MyOp"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "myop"; }
    [[nodiscard]] int              input_count() const noexcept override { return 2; }

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};
```

**Step 2 — Implement** in `src/lib/signal_ops.cpp`.

**Step 3 — Register** in `create_operation_registry()` (`src/lib/operation_registry.cpp`):

```cpp
OperationRegistry create_operation_registry() {
    OperationRegistry reg;
    reg.register_type<AddOp>("add");
    // ...
    reg.register_type<MyOp>("myop");   // ← add this
    return reg;
}
```

---

## 6. How to Add a New Visualization Tool {#new-viz-tool}

Visualization tools render interactive overlays on top of the active plot.
They inherit from `IVisualizationTool`.

**Step 1 — Create header** `include/gnc_viz/my_tool.hpp`:

```cpp
#pragma once
#include "gnc_viz/interfaces.hpp"

namespace gnc_viz {

class MyTool : public IVisualizationTool {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "My Tool"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "mytool"; }
    [[nodiscard]] std::string_view icon() const noexcept override { return "M"; }

    void handle_input(AppState& state) override;
    void render_overlay(const AppState& state) override;
    void on_activate() override;
    void on_deactivate() override;
};

} // namespace gnc_viz
```

**Step 2 — Implement** in `src/lib/my_tool.cpp`.

**Step 3 — Register** in `ToolManager` constructor (`src/lib/tool_manager.cpp`):

```cpp
ToolManager::ToolManager() {
    m_registry.register_type<AnnotationTool>("annotation");
    m_registry.register_type<RulerTool>("ruler");
    m_registry.register_type<MyTool>("mytool");   // ← add this
}
```

---

## 7. The Registry<T> Template {#registry}

`Registry<Interface>` (in `include/gnc_viz/registry.hpp`) maps string keys to factory
functions that produce `unique_ptr<Interface>`.

```cpp
// Registration
PlotRegistry reg;
reg.register_type<TimeSeriesPlot>("timeseries");
reg.register_factory("custom", []{ return std::make_unique<CustomPlot>(); });

// Instantiation
auto plot = reg.create("timeseries");  // → unique_ptr<IPlotType>

// Introspection
reg.keys();     // → {"timeseries", "custom"} (insertion order)
reg.size();     // → 2
reg.contains("timeseries");  // → true
```

Three convenience type aliases are predefined:

```cpp
using PlotRegistry      = Registry<IPlotType>;
using OperationRegistry = Registry<ISignalOperation>;
using ToolRegistry      = Registry<IVisualizationTool>;
```

---

## 8. SignalBuffer Memory Model {#signal-buffer-memory}

`SignalBuffer` stores a time array and a flat row-major value array for one HDF5 dataset.

```
time[N]           values[N × K]
t0                v[0][0], v[0][1], …, v[0][K-1]
t1                v[1][0], v[1][1], …, v[1][K-1]
…                 …
t(N-1)            v[N-1][0], …, v[N-1][K-1]
```

Buffers are owned by `shared_ptr`.  `SimulationFile` keeps a `weak_ptr` cache; if all
`PlottedSignal`s referencing a buffer are removed, the buffer is automatically freed.

Call `SignalBuffer::evict()` to drop the data arrays while keeping the `SignalBuffer`
object alive (metadata survives for UI display).

---

## 9. Testing (Catch2) {#testing}

Tests live under `tests/` and use [Catch2 v3](https://github.com/catchorg/Catch2).

```bash
# Build and run tests
cmake --build build --target gnc_viz_tests
./build/gnc_viz_tests

# Run a specific test by name/tag
./build/gnc_viz_tests "[signal_buffer]"
./build/gnc_viz_tests "[registry]"

# Run with verbose output
./build/gnc_viz_tests -v
```

### Writing a new test

```cpp
// tests/test_my_component.cpp
#include <catch2/catch_test_macros.hpp>
#include "gnc_viz/my_component.hpp"

TEST_CASE("MyComponent does X", "[my_component]") {
    MyComponent c;
    REQUIRE(c.foo() == 42);
}
```

Add the file to the `gnc_viz_tests` target in `tests/CMakeLists.txt`.

---

## 10. Session JSON Schema {#session-schema}

Session files (default path `~/.gnc_viz/session.json`) use the following schema:

```json
{
  "version": 1,
  "simulations": [
    { "path": "/data/sim_2025_01.h5", "sim_id": "sim0", "display_name": "Nominal" }
  ],
  "plotted_signals": [
    {
      "signal_key": "sim0//attitude/quaternion_body",
      "alias": "q_body",
      "color": "0x4477AAFF",
      "y_axis_index": 0,
      "visible": true
    }
  ],
  "y_axes": [
    { "label": "Quaternion", "min": -1.1, "max": 1.1, "auto_range": true }
  ],
  "zoom": { "x_min": 0.0, "x_max": 100.0 },
  "active_plot_type": "timeseries",
  "annotations": [
    { "time": 42.5, "value": 0.707, "text": "Apogee", "has_arrow": true }
  ],
  "panes": {
    "file_pane_visible": true,
    "signal_pane_visible": true,
    "file_pane_width": 280.0,
    "signal_pane_width": 300.0
  }
}
```

---

## 11. Error Handling (gnc::Result<T>) {#error-handling}

All fallible public API functions return `gnc::Result<T>` (`std::expected<T, gnc::Error>`).

```cpp
// Return a successful value
return std::make_shared<SignalBuffer>(...);

// Return an error
return gnc::make_error<std::shared_ptr<SignalBuffer>>(
    gnc::ErrorCode::HDF5ReadFailed,
    "Cannot read dataset",
    "/attitude/quaternion");

// Propagate errors with GNC_TRY
gnc::Result<Foo> my_func() {
    auto x = GNC_TRY(step_one());
    auto y = GNC_TRY(step_two(x));
    return Foo{x, y};
}

// Handle errors at the call site
auto result = load_signal(meta);
if (!result) {
    GNC_LOG_WARN("Load failed: {}", result.error().to_string());
    return;
}
auto& buf = result.value();
```
