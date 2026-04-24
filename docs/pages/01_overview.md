@page overview Overview
@brief FastScope — HDF5 Data Visualizer

# FastScope

FastScope is an interactive macOS desktop application for visualizing HDF5 data files containing
time-series signals.  It lets you quickly explore large datasets without writing any scripts.

---

## Purpose

Engineering and scientific tools often produce HDF5 files containing hundreds of signals —
scalars, vectors, and matrices — sampled at varying rates across long data collection runs.

FastScope solves the "open it fast, look at everything" problem:

- **Zero-configuration file loading**: drop an HDF5 file on the window — done.
- **Hierarchical signal browser**: explore the full H5 tree, search, multi-select.
- **Multi-Y-axis time-series plots**: compare signals with different units side-by-side.
- **Derived signals**: compute norms, differences, sums, or scaled variants on the fly.
- **Session save/restore**: reopen the application and find your workspace exactly as you left it.

---

## Feature List

| Category | Feature |
|----------|---------|
| **File I/O** | HDF5 file loading (drag-and-drop or Cmd+O), multiple files simultaneously, per-file time-axis selection |
| **Signal Browser** | Hierarchical tree with full H5 paths, live search/filter, signal metadata (units, shape, dtype) |
| **Time Series** | Multi-Y-axis (up to 3), colored lines, auto-fit, crosshair + value tooltip |
| **Derived Signals** | Add, subtract, multiply, scale, magnitude operations; results cached |
| **Tools** | Annotation tool (click to pin a label), Ruler tool (two-point distance) |
| **Export** | CSV export (all visible signals), PNG screenshot via macOS screencapture |
| **Session** | JSON session file: re-opens files, restores plotted signals, zoom, annotations |
| **UI** | Collapsible side panes, dark theme, dockable windows |

---

## Architecture Overview (ASCII)

```
┌─────────────────────────────────────────────────────────────────────┐
│                         macOS Application                           │
│                                                                     │
│  ┌──────────────┐   ┌─────────────────────┐   ┌───────────────┐   │
│  │  Files Pane  │   │   Signals Pane       │   │  Plot Area    │   │
│  │              │   │                     │   │               │   │
│  │ SimulationFile│   │ SignalTreeWidget     │   │  PlotEngine   │   │
│  │ (HDF5Reader) │   │ render_signal_tree() │   │  ┌──────────┐ │   │
│  │              │   │                     │   │  │IPlotType │ │   │
│  └──────┬───────┘   └──────────┬──────────┘   │  │(active)  │ │   │
│         │                      │              │  └──────────┘ │   │
│         └──────────────────────┘              └───────────────┘   │
│                                │                      │            │
│                         ┌──────▼──────────────────────▼───────┐   │
│                         │              AppState                │   │
│                         │  simulations / plotted_signals       │   │
│                         │  axis_manager / tool_manager         │   │
│                         │  colors / panes / debug              │   │
│                         └──────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Screenshot

*(Screenshot placeholder — add `docs/assets/screenshot.png` and update the path below.)*

```
[Main window screenshot would appear here]
```

---

## Quick Start

```bash
# Install dependencies
brew install hdf5 cmake

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Run
./build/fastscope path/to/data.h5
```

---

## Related Pages

- @ref user_manual       — Step-by-step usage guide
- @ref developer_manual  — Guide for contributors extending FastScope
- @ref architecture      — Component diagram and data-flow reference
