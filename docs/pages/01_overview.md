@page overview Overview
@brief GNC Viz — Spacecraft GNC Simulation Data Visualizer

# GNC Viz

GNC Viz is an interactive macOS desktop application for visualizing HDF5 output files produced
by spacecraft Guidance, Navigation & Control (GNC) simulations.  It is aimed at spacecraft
engineers who need to quickly explore large time-series datasets without writing any scripts.

---

## Purpose

Modern spacecraft GNC simulators (e.g. MATLAB/Simulink, Python-based Monte Carlo environments,
or C++ batched sims) produce HDF5 files containing hundreds of signals — quaternions, Euler
angles, angular rates, position vectors, velocity, thruster commands, and sensor readings — all
sampled at varying rates across a simulation run that may span several hours of mission time.

GNC Viz solves the "open it fast, look at everything" problem:

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
| **Trajectory 2D** | XY / YZ / XZ scatter plot with axis-pair selector |
| **Ground Track** | Lat/Lon scatter on a plain axes, computed from ECI position vectors |
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
./build/gnc_viz path/to/simulation.h5
```

---

## Related Pages

- @ref user_manual       — Step-by-step usage guide for spacecraft engineers
- @ref developer_manual  — Guide for contributors extending GNC Viz
- @ref architecture      — Component diagram and data-flow reference
