# FastScope — GitHub Copilot Instructions

This file gives GitHub Copilot (and other AI coding agents) the project-specific context needed to
work in this repository without additional prompting.

---

## What is FastScope?

FastScope is a **macOS desktop application** for visualizing data from HDF5 files.
It is a **generic, domain-agnostic tool** — it works with any HDF5 file containing time-series or
multidimensional datasets. Do **not** reference GNC, spacecraft, guidance, navigation, or control in
user-facing strings, comments, documentation, or variable names. The software is equally useful for
any engineering or scientific domain.

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Language | C++23 (OBJCXX for macOS bridge code) |
| GUI | Dear ImGui + ImPlot (fetched via CMake FetchContent) |
| Renderer | Metal (via GLFW + imgui_impl_metal) |
| Data files | HDF5 (system `brew install hdf5`) |
| Build | CMake 3.25+, Apple Clang |
| Tests | Catch2 v3 (FetchContent) |
| Platform | **macOS arm64 only** — no Windows/Linux support planned |

---

## Project Layout

```
include/fastscope/     ← All public headers (namespace fastscope::)
src/app/               ← UI + rendering: Application, PlotEngine, plot types, tools
src/lib/               ← Pure-logic library (no UI): HDF5 reader, signal ops, registries
src/tests/             ← Catch2 unit tests (test fastscope_lib, not the app)
docs/pages/            ← Hand-written Doxygen pages (overview, user manual, dev manual)
docs/doxygen-awesome/  ← CSS theme for Doxygen HTML
.github/workflows/     ← docs.yml: builds Doxygen and deploys to GitHub Pages
```

---

## Build & Test

```bash
# Configure (first time)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j$(sysctl -n hw.ncpu)

# Run app
./build/bin/fastscope

# Run tests (all 555+ must pass)
./build/bin/fastscope_tests

# Build documentation
cmake --build build --target docs
# Output: build/docs/html/index.html

# Clean build
rm -rf build && cmake -B build && cmake --build build -j$(sysctl -n hw.ncpu)
```

**Never commit if `./build/bin/fastscope_tests` has failures.**

---

## CMake Targets

| Target | Description |
|--------|-------------|
| `fastscope_lib` | Pure-logic library (HDF5, signal processing, registries) |
| `fastscope` | GUI application (links `fastscope_lib`) |
| `fastscope_tests` | Catch2 test binary (tests `fastscope_lib`) |
| `docs` | Doxygen HTML documentation |

New **plot types and tools** go in `src/app/` and link against `fastscope_lib`.
New **signal operations, registries, data readers** go in `src/lib/`.

---

## Namespace & Include Convention

```cpp
// All production code uses the fastscope:: namespace
namespace fastscope { ... }

// Headers always use the full include path
#include "fastscope/app_state.hpp"
#include "fastscope/interfaces.hpp"
#include "fastscope/error.hpp"
```

---

## Error Handling (no exceptions in library layer)

```cpp
// Result<T> is an alias for std::expected<T, fastscope::Error>
fastscope::Result<std::shared_ptr<SignalBuffer>> load_signal(const SignalMetadata& meta);

// Propagate with FASTSCOPE_TRY (expands to std::expected unwrap + early return)
auto buf = FASTSCOPE_TRY(reader.load_signal(meta));

// Create errors
return std::unexpected(fastscope::Error{fastscope::ErrorCode::FileNotFound, "path not found", path});

// UI layer converts errors to dialogs — never catch in library code
```

Key rule: **UI (application.mm, signal_tree_widget.cpp, etc.) may call `.error()` and show dialogs.
Library code (`src/lib/`) must never catch or swallow errors — propagate with `FASTSCOPE_TRY` or return
`std::unexpected`.**

---

## Memory & Ownership Rules

1. **No raw owning pointers.** Use `unique_ptr` (sole ownership) or `shared_ptr` (shared ownership).
2. **Registry<T> returns `unique_ptr<Interface>`.** Callers own the result.
3. **`SignalBuffer` is `shared_ptr<SignalBuffer>`.** Multiple `PlottedSignal` instances share the
   same buffer; `SimulationFile` holds a `weak_ptr` cache — buffers are reloaded if evicted.
4. **`AppState` is owned by `Application` and passed by non-const reference to subsystems.**
   Never store a pointer to AppState; always use the ref passed into the method.

---

## Performance Rules (render loop)

These are **hard rules** — violating them causes frame drops (target: 60 fps):

1. **No heap allocation in `render()` or `RunFrame()`.** Pre-allocate everything in `build_component_cache()`.
2. **Float caches are mandatory.** Signal data is `double` internally; render code uses pre-converted
   `vector<float>` caches (`time_cache_f`, `component_cache` in `PlottedSignal`).
3. **Decimation is active.** `TimeSeriesPlot::render()` uses stride + visible-range culling to cap
   rendered points to ≈ 2× plot pixel width. Do not bypass this for new plot types.
4. **ImPlot stride API.** Use `ImPlotPoint` stride via `ImPlot::PlotLine(..., stride)` rather than
   copying subsets into temporary vectors.

---

## The Three Extension Interfaces

All defined in `include/fastscope/interfaces.hpp`.

### 1. `IPlotType` — a way to visualize data

```cpp
class IPlotType {
public:
    virtual std::string_view name() const noexcept = 0;   // "Time Series"
    virtual std::string_view id()   const noexcept = 0;   // "timeseries"
    virtual void render(AppState& state, float w, float h) = 0;
    virtual void on_activate(AppState& state) {}
    virtual void on_deactivate() {}
};
```

Register in `PlotEngine::PlotEngine()` (`src/app/plot_engine.cpp`).
See `.github/prompts/add-plot-type.prompt.md` for the full recipe.

### 2. `ISignalOperation` — transforms N signals into 1

```cpp
class ISignalOperation {
public:
    virtual std::string_view name()        const noexcept = 0;  // "Magnitude"
    virtual std::string_view id()          const noexcept = 0;  // "magnitude"
    virtual int              input_count() const noexcept = 0;  // -1 = variadic
    virtual fastscope::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) = 0;
};
```

Register in `create_operation_registry()` (`src/lib/operation_registry.cpp`).
See `.github/prompts/add-signal-op.prompt.md`.

### 3. `IVisualizationTool` — interactive overlay on the plot

```cpp
class IVisualizationTool {
public:
    virtual std::string_view name() const noexcept = 0;  // "Ruler"
    virtual std::string_view id()   const noexcept = 0;  // "ruler"
    virtual std::string_view icon() const noexcept = 0;  // single char for toolbar
    virtual void handle_input(AppState& state) = 0;
    virtual void render_overlay(const AppState& state) = 0;
    virtual void on_activate() {}
    virtual void on_deactivate() {}
};
```

Register in `ToolManager::ToolManager()` (`src/app/tool_manager.cpp`).
See `.github/prompts/add-tool.prompt.md`.

---

## Registry<T> Template

Defined in `include/fastscope/registry.hpp`.

```cpp
PlotRegistry reg;
reg.register_type<TimeSeriesPlot>("timeseries");        // registers factory
auto plot = reg.create("timeseries");                   // → unique_ptr<IPlotType>
reg.keys();                                              // → {"timeseries"} in order
reg.contains("timeseries");                              // → true
```

---

## AppState — Central Data Model

Defined in `include/fastscope/app_state.hpp`. Single instance in `Application`.

Key fields:
```cpp
struct AppState {
    std::vector<SimulationFile>   simulations;       // loaded .h5 files
    std::vector<PlottedSignal>    plotted_signals;   // signals currently on the plot
    AxisManager                   axis_manager;      // Y-axis assignments
    ToolManager                   tool_manager;      // active visualization tool
    ColorManager                  color_manager;     // per-signal colors
    // UI state
    struct Panes { bool left_collapsed, middle_collapsed; float left_w, middle_w; } panes;
};
```

---

## PlottedSignal — Rendering Data

Defined in `include/fastscope/plotted_signal.hpp`.

```cpp
struct PlottedSignal {
    std::string               sim_id;
    std::string               sim_name;       // user-editable simulation name
    SignalMetadata            metadata;
    std::string               alias;          // user-editable display name
    std::vector<float>        time_cache_f;   // pre-converted float time data
    std::vector<std::vector<float>> component_cache;  // one vector per component
    std::weak_ptr<SignalBuffer>     cache_source;
    ImVec4                    color;
    bool                      visible = true;
    int                       y_axis  = 0;

    std::string plot_key()     const;   // "<sim_id>/<path>"
    std::string display_name() const;   // "SimName / alias-or-path"
};
```

The `display_name()` includes the simulation name — **do not suppress this in legends**.

---

## Code Style

- **C++23 features are encouraged**: `std::expected`, `std::span`, `std::ranges`, structured bindings.
- `[[nodiscard]]` on all functions returning `Result<T>`, `unique_ptr`, or `shared_ptr`.
- `noexcept` on all getters and pure observers.
- `const` on all read-only methods.
- Doxygen `///` comments on every `public` member in headers (`include/fastscope/`).
- No `using namespace std;` anywhere.
- Prefer `std::string_view` over `const std::string&` for read-only string parameters.
- `.mm` extension for Objective-C++ files (macOS bridge, ImGui backend, file dialogs).

---

## Branding Rules

- **Application name**: FastScope
- **Window title**: "FastScope"
- **Binary name**: `fastscope`
- **Namespace**: `fastscope::`
- **Never use**: GNC, spacecraft, guidance, navigation, control, satellite, orbit, telemetry
  in user-facing strings, UI labels, comments, or variable names.
  The software is a generic HDF5 time-series visualizer.

---

## Doxygen Documentation

Live at: https://maltergate.github.io/curly-giggle/

- Built automatically by `.github/workflows/docs.yml` on every push to `main`.
- Local build: `cmake --build build --target docs`
- All `public` members in `include/fastscope/` must have `///` Doxygen comments.
- Hand-written pages are in `docs/pages/` (Markdown).
- **Do not modify** `docs/doxygen-awesome/` — it is a vendored CSS theme.

---

## Common Pitfalls

| Pitfall | Correct approach |
|---------|-----------------|
| Allocating temp vectors in render() | Pre-allocate in `build_component_cache()` |
| Adding raw `new` / `delete` | Use `make_unique` / `make_shared` |
| Throwing exceptions in `src/lib/` | Return `std::unexpected(...)` |
| Hard-coding pixel widths for panes | Use `AppState.panes.*_w` floats |
| Adding a 4th Y-axis | ImPlot max is 3; document and warn user |
| Forgetting to add `.cpp` to CMakeLists.txt | Always update `src/app/CMakeLists.txt` or `src/lib/CMakeLists.txt` |
| Writing "GNC" or "spacecraft" in UI strings | Use generic terms: "dataset", "signal", "file" |
| Bypassing decimation for new plot types | Implement stride + culling like `timeseries_plot.cpp` |
