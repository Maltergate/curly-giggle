# FastScope — Architecture

## Introduction

FastScope is a C++23 / Dear ImGui macOS desktop application for loading,
comparing, and visualising HDF5 output files from GNC (Guidance, Navigation &
Control) spacecraft simulations.  It lets engineers open multiple `.h5` run
files side-by-side, browse the full signal hierarchy, drag signals onto
multi-Y-axis time-series plots, run derived-signal computations, annotate
plots, and export results to CSV/PNG.

---

## High-Level Module Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│  Application (application.mm)  ←  GLFW + Metal + ImGui frame loop      │
│                                                                         │
│  ┌───────────────┐  ┌──────────────────┐  ┌──────────────────────────┐ │
│  │  File pane    │  │  Signal tree     │  │  Plot pane               │ │
│  │  (Simulation  │  │  (SignalTree     │  │  ┌────────────────────┐  │ │
│  │   ListUI)     │  │   Widget)        │  │  │  PlotEngine        │  │ │
│  └──────┬────────┘  └──────┬───────────┘  │  │  (IPlotType)       │  │ │
│         │                  │              │  └────────────────────┘  │ │
│         │                  │              │  ToolManager / toolbar   │ │
│         └──────────────────┴──────────────┤  Status bar              │ │
│                             AppState      └──────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────┬──┘
                                                                       │
           ┌───────────────────────────────────────────────────────────┘
           │            fastscope_lib  (pure C++, no UI)
           ▼
┌──────────────────────────────────────────────────────────────────────────┐
│  SimulationFile → HDF5Reader → SignalBuffer                              │
│  AxisManager  ·  TimeAligner  ·  DerivedSignal  ·  SignalOps             │
│  ColorManager  ·  Session  ·  Config  ·  CSVExporter  ·  PNGExporter    │
└──────────────────────────────────────────────────────────────────────────┘
```

The codebase is split into two CMake targets:

| Target         | Language         | Key deps                       | Role                       |
|----------------|------------------|--------------------------------|----------------------------|
| `fastscope_lib`  | C++23            | HDF5, spdlog, nlohmann-json    | Data layer — fully testable without a display |
| `fastscope`      | C++23 + ObjC++   | ImGui, ImPlot, Metal, GLFW     | UI application              |

---

## Key Classes

### `AppState` (`include/fastscope/app_state.hpp`)
Central data model.  A single instance lives inside `Application::Impl`.
Contains:
- `simulations` — open HDF5 files (`std::vector<unique_ptr<SimulationFile>>`)
- `plotted_signals` — signals currently on the plot (`std::vector<PlottedSignal>`)
- `axis_manager` — Y-axis assignment and config
- `tool_manager` — active annotation / ruler tool
- `colors` — per-signal color palette
- `panes` — layout widths and visibility flags
- `debug` — ImGui / ImPlot demo toggles

AppState is not copyable (owns `unique_ptr` simulations).

### `PlotEngine` (`include/fastscope/plot_engine.hpp`)
Owns a `PlotRegistry` and the currently active `IPlotType`.  Currently active
type: `timeseries`.  Types `trajectory2d` and `groundtrack` are implemented but
not yet registered (pending polish).  Delegates `render()`, `on_activate()`, and
`on_deactivate()` to the active type.  Exposes `fit_to_data()` / `fit_x_only()`
as conveniences for the toolbar; these delegate via the `IPlotType::request_fit()`
virtual method (no `dynamic_cast` in the engine).

### `AxisManager` (`include/fastscope/axis_manager.hpp`)
Tracks which plotted signal (by `plot_key`) is on which Y axis (0–2) and
stores the `AxisConfig` (label, units, range) for each axis.
- `assign(key, axis_id)` — add / update assignment
- `clear_assignments()` — clear assignments without touching `AxisConfig`s (used for per-frame re-sync without data loss)
- `clear()` — reset both assignments and configs (used when all signals removed)
- `active_axes()` — returns configs for axes that have at least one signal (axis 0 always included)

### `TimeAligner` (`include/fastscope/time_aligner.hpp`)
Resamples N `SignalBuffer`s onto a common uniform time grid using linear
interpolation.  Fast path: if all inputs share the same time vector, copies are
returned without interpolation.  Used by `DerivedSignal` and future
multi-simulation comparisons.

### `ToolManager` (`include/fastscope/tool_manager.hpp`)
Registry of `IVisualizationTool` factories.  `activate(id)` instantiates a
tool or toggles it off if it is already active.  `tick(state)` is called once
per frame from inside the `BeginPlot` block to let the tool draw overlays.
Built-in tools: `annotation`, `ruler`.

### `SimulationFile` (`include/fastscope/simulation_file.hpp`)
Wraps `HDF5Reader` with a `sim_id`, a user-selected time axis, and a
`weak_ptr` buffer cache.  `load_signal(meta)` loads (or returns a cached)
`SignalBuffer`.  Signals are enumerated lazily on `open()`.

### `SignalBuffer` (`include/fastscope/signal_buffer.hpp`)
Immutable, ref-counted time + values storage.  Supports scalar and vector
signals (N components).  Exposes `time()`, `values()`, `at(i)`, `at(i, k)`,
`sample_count()`, `n_components()`.  Created via `SignalBuffer::make_vector()`
or `SignalBuffer::make_span()` (zero-copy from a mapped HDF5 buffer).

### `DerivedSignal` (`include/fastscope/derived_signal.hpp`)
Computes a new `SignalBuffer` from N input buffers using a registered
`ISignalOperation`.  Built-in operations (via `OperationRegistry`):
`add`, `subtract`, `multiply`, `scale`, `magnitude`.

### `Session` (`include/fastscope/session.hpp`)
JSON serialisation / deserialisation of `AppState`.  Schema version 1.
`save_session()` writes pane state, plotted-signal metadata (not buffer data),
and axis configs.  `load_session()` restores them; missing simulation files are
silently skipped.

---

## Data Flow

```
HDF5 file on disk
      │
      ▼
 SimulationFile::open()
      │  (HDF5Reader enumerates datasets → vector<SignalMetadata>)
      ▼
 SimulationFile::load_signal(meta)
      │  (HDF5Reader reads dataset → SignalBuffer)
      ▼
 PlottedSignal::buffer  ←  shared_ptr<SignalBuffer>
      │
      ├─ build_component_cache()     ← one-time extraction of per-component
      │   component_cache[k]            columnar float64 arrays
      │
      ▼
 TimeSeriesPlot::render()
      │  (dirty-flag axis sync, then ImPlot::PlotLine per signal/component)
      ▼
 ImPlot → Metal GPU → screen
```

### Per-frame render hot path (TimeSeriesPlot)

1. For each `PlottedSignal`: if buffer not loaded, call `load_signal()`.
2. If `component_cache` is stale (`cache_source.lock() != buffer`), rebuild it
   once — this is the only per-signal allocation, done once per load.
3. **Axis sync (dirty flag)**: build a fingerprint `(plot_key, y_axis)` for all
   signals; only call `axis_manager.clear_assignments()` + `assign()` when it
   changes.  This avoids O(N) unordered_map churn every frame.
4. `ImPlot::BeginPlot` → `SetupAxis` → per-signal `PlotLine` → `EndPlot`.
   Labels use stack-allocated `char[512]` buffers — no heap allocation.

---

## Extension Points

### Adding a new plot type

1. Create `include/fastscope/my_plot.hpp` and `src/app/my_plot.cpp`.
2. Inherit from `IPlotType` (in `include/fastscope/interfaces.hpp`).
3. Implement `name()`, `id()`, `render()`, `on_activate()`, `on_deactivate()`.
4. Register in `PlotEngine::PlotEngine()`:
   ```cpp
   m_registry.register_type<MyPlot>("myplot");
   ```
5. No other files need to change.

### Adding a new tool

1. Create header + source in `include/fastscope/` and `src/app/`.
2. Inherit from `IVisualizationTool`.
3. Register the factory in `ToolManager::ToolManager()`.
4. Add the source to `src/app/CMakeLists.txt` and `src/tests/CMakeLists.txt`
   (tools are included in the test binary for registry tests).

### Adding a new signal operation

1. Create a class implementing `ISignalOperation` (`apply(inputs) → Result<SignalBuffer>`).
2. Register in `create_operation_registry()` (`src/lib/operation_registry.cpp`).
3. The derived-signal UI in `application.mm` picks it up automatically.

---

## Testing Approach

All logic in `fastscope_lib` is tested headlessly via **Catch2 v3** in
`src/tests/`.  The test binary (`fastscope_tests`) links:
- `fastscope_lib` — full data layer
- `imgui_lib` + `implot_lib` — needed for `ToolManager` + `AnnotationTool`
- Selected `src/app/` sources: `tool_manager.cpp`, `annotation_tool.cpp`,
  `ruler_tool.cpp`

No ImGui context is created in tests; tool constructor/register paths work
without one.

Test files by area:

| File | Area |
|---|---|
| `test_signal_buffer.cpp` | SignalBuffer construction, access, edge cases |
| `test_time_aligner.cpp` | interpolation, grid, fast path |
| `test_axis_manager.cpp` | assign, release, clear, config mutation |
| `test_session.cpp` | save / load round-trips |
| `test_tool_manager.cpp` | activate, toggle, deactivate |
| `test_system.cpp` | end-to-end workflow integration tests |
| … | … |

Run all tests:
```bash
./build/bin/fastscope_tests
# or
ctest --test-dir build -V
```
