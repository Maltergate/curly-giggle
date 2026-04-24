@page architecture Architecture Reference
@brief Component diagram and data flow

# Architecture Reference

This page provides an in-depth view of FastScope's component structure, data flow,
threading model, error handling strategy, and extension points.

---

## Component Diagram (ASCII)

```
╔══════════════════════════════════════════════════════════════════════════╗
║  Application  (application.hpp / application.mm)                        ║
║  GLFW window  ·  Metal device  ·  ImGui context  ·  render loop         ║
║                                                                          ║
║  ┌──────────────────────────────────────────────────────────────────┐   ║
║  │                         AppState                                  │   ║
║  │                                                                   │   ║
║  │  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐  │   ║
║  │  │   Data Layer      │  │   Plot Layer      │  │  Tool Layer   │  │   ║
║  │  │                  │  │                  │  │               │  │   ║
║  │  │ SimulationFile[] │  │ PlottedSignal[]  │  │ ToolManager   │  │   ║
║  │  │   HDF5Reader     │  │ AxisManager      │  │  AnnotationT. │  │   ║
║  │  │   SignalBuffer   │  │                  │  │  RulerTool    │  │   ║
║  │  │   SignalMetadata │  │                  │  │               │  │   ║
║  │  └──────────────────┘  └──────────────────┘  └───────────────┘  │   ║
║  │                                                                   │   ║
║  │  ┌──────────────┐  ┌──────────────────────┐  ┌───────────────┐  │   ║
║  │  │ UI / Layout  │  │  Signal Ops           │  │  Utilities    │  │   ║
║  │  │              │  │                      │  │               │  │   ║
║  │  │ PaneState    │  │ DerivedSignal        │  │ ColorManager  │  │   ║
║  │  │ DebugState   │  │ OperationRegistry    │  │ Config        │  │   ║
║  │  └──────────────┘  └──────────────────────┘  └───────────────┘  │   ║
║  └──────────────────────────────────────────────────────────────────┘   ║
║                                                                          ║
║  ┌───────────────────┐   ┌──────────────────────────────────────────┐   ║
║  │  UI Widgets       │   │  PlotEngine                              │   ║
║  │  SimulationListUI │   │  PlotRegistry                            │   ║
║  │  SignalTreeWidget │   │   TimeSeriesPlot  (active)               │   ║
║  │  CollapsiblePane  │   │   Trajectory2DPlot  (pending polish)     │   ║
║  └───────────────────┘   │   GroundTrackPlot   (pending polish)     │   ║
║                           └──────────────────────────────────────────┘   ║
╚══════════════════════════════════════════════════════════════════════════╝
```

---

## Data Flow

### File Open → Signal Enumeration → Plot Render

```
1. USER: Drop .h5 file / press Cmd+O
           │
           ▼
2. show_open_dialog()  →  path : filesystem::path
           │
           ▼
3. SimulationFile::open(path)
           │  HDF5Reader::open(path)          ← opens libhdf5 file handle
           │  HDF5Reader::enumerate_signals() ← walks all H5 groups/datasets
           │  HDF5Reader::suggest_time_axes() ← heuristic: 1-D "time" datasets
           │
           ▼
4. AppState::simulations.push_back(sim)
   signals() = vector<SignalMetadata>  ← only metadata; no data loaded yet

           │
           ▼
5. render_simulation_list() / render_signal_tree()
   ← shows tree of SignalMetadata nodes in middle pane

           │ USER: clicks [+] next to a signal
           ▼
6. AppState::plotted_signals.push_back(PlottedSignal{
       sim_id, meta, buffer=nullptr,
       color = ColorManager::assign(plot_key) })

           │
           ▼
7. Next frame: PlotEngine::render(state, w, h)
           │  TimeSeriesPlot::render(state, w, h)
           │    for each ps in plotted_signals where ps.buffer == nullptr:
           │      buf = SimulationFile::load_signal(ps.meta)
           │             ← HDF5Reader::load_signal()   HDF5 data read here
           │             ← builds SignalBuffer (time[], values[N×K])
           │      ps.buffer = buf
           │      build ps.time_cache_f, ps.component_cache (float arrays)
           │    ImPlot::BeginPlot(...)
           │    for each ps in plotted_signals:
           │      ImPlot::PlotLine(label, xs, ys, N)
           │    ImPlot::EndPlot()
           │
           ▼
8. Metal: frame is presented to screen
```

### Buffer Lifetime

```
PlottedSignal::buffer  ──shared_ptr──►  SignalBuffer
                                              ▲
SimulationFile::m_cache ──weak_ptr──────────┘

When last PlottedSignal referencing a buffer is removed:
  shared_ptr refcount → 0  →  SignalBuffer destroyed automatically.
  weak_ptr in cache becomes expired (no-op on next access).
```

---

## Thread Model

FastScope is **strictly single-threaded**.

```
Main Thread
  │
  ├─ Metal render loop (60 fps target)
  │    ├─ ImGui::NewFrame()
  │    ├─ render UI (all three panes)
  │    ├─ PlotEngine::render()     ← may trigger HDF5 reads
  │    ├─ ImGui::Render()
  │    └─ Metal present
  │
  └─ Event loop (GLFW callbacks: key, mouse, file drop)
```

HDF5 reads happen **inside the render loop** on the main thread (lazy, first-frame load).
For large files (> ~100 MB) this causes a one-frame stall.  A background I/O thread
is planned for a future version; the `SignalBuffer` API is designed to support it
(buffers become non-null atomically from the main thread).

---

## Error Handling

All fallible functions return `fastscope::Result<T>` — an alias for `std::expected<T, fastscope::Error>`.
The application **never throws** outside of unit tests.

```
fastscope::Result<T>
  = std::expected<T, fastscope::Error>

fastscope::Error {
  ErrorCode  code     // machine-readable category
  string     message  // human-readable explanation
  string     context  // optional: file path, dataset name, etc.
}
```

Error categories:

| Range | Category |
|-------|---------|
| 0–99 | Generic (Unknown, NotFound, IOError, …) |
| 100–199 | File I/O (FileNotFound, FileOpenFailed, FileReadFailed) |
| 200–299 | HDF5 (HDF5OpenFailed, HDF5ReadFailed, HDF5TypeMismatch) |
| 300–399 | Signal processing (SignalNotFound, DimensionMismatch, …) |
| 400–499 | Rendering / UI (RenderFailed) |

Errors that are non-fatal (e.g. failed session load) are logged with `FASTSCOPE_LOG_WARN`
and execution continues with sensible defaults.

---

## Extension Points Summary

| Extension point | Interface | Registered in | Key method |
|-----------------|-----------|---------------|------------|
| Plot type | `IPlotType` | `PlotEngine` constructor | `render()` |
| Signal operation | `ISignalOperation` | `create_operation_registry()` | `execute()` |
| Visualization tool | `IVisualizationTool` | `ToolManager` constructor | `handle_input()` + `render_overlay()` |

All three use the same `Registry<Interface>` template: register a type with
`register_type<T>(key)`, create instances with `create(key)`.

---

## Module Groups

| Group | Headers |
|-------|---------|
| `core_interfaces` | interfaces.hpp, registry.hpp, error.hpp |
| `data_layer` | signal_buffer.hpp, signal_metadata.hpp, simulation_file.hpp, hdf5_reader.hpp, time_aligner.hpp |
| `plot_system` | plot_engine.hpp, timeseries_plot.hpp, trajectory_2d_plot.hpp, ground_track_plot.hpp, axis_manager.hpp |
| `tools` | tool_manager.hpp, annotation_tool.hpp, ruler_tool.hpp |
| `signal_ops` | signal_ops.hpp, operation_registry.hpp, derived_signal.hpp |
| `app_layer` | app_state.hpp, plotted_signal.hpp, application.hpp |
| `ui_widgets` | signal_tree_widget.hpp, simulation_list_ui.hpp, collapsible_pane.hpp |
| `utilities` | color_manager.hpp, config.hpp, session.hpp, log.hpp, version.hpp, file_open_dialog.hpp, csv_exporter.hpp, png_exporter.hpp |
