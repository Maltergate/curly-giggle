# GNC Viz — Spacecraft Simulation Data Visualizer

An interactive **C++23 / Dear ImGui** desktop application for loading and comparing HDF5 output
files from GNC (Guidance, Navigation & Control) spacecraft simulations.

> **Current status:** v1 — full feature set complete.
> Load a file → browse the signal tree → click to plot → annotate and export.

<!-- Screenshot placeholder -->
<!-- ![GNC Viz screenshot](docs/screenshot.png) -->

---

## Features

- **Multi-file comparison** — load several `.h5` simulation runs side-by-side
- **Signal browser** — collapsible HDF5 tree with full path display and live search filter
- **Time-series plot** — multi-Y-axis (left / right / Y3), color-coded lines, crosshair tooltip
- **Trajectory plot** — 2-D XY / XZ / YZ projection of position vectors
- **Ground-track plot** — lat/lon overlay
- **Derived signals** — add, subtract, scale, vector magnitude computed on the fly
- **Tools** — annotation markers and ruler with on-plot measurement
- **Alias editor** — double-click any plotted signal to rename its legend label
- **Y-axis control** — right-click an axis to set label, range, and auto-fit
- **Fit All / Fit X** — one-click re-zoom toolbar buttons
- **CSV export** — dump all plotted signals to a time-aligned CSV
- **PNG export** — screenshot the current plot window
- **Session save/restore** — JSON file preserves open files, plotted signals, pane layout
- **Status bar** — live FPS, simulation count, plotted-signal count
- **Dark theme** — `ImGui::StyleColorsDark`
- **Drag-and-drop** — drop `.h5` / `.hdf5` files directly onto the window

---

## Requirements

### macOS (primary platform)

| Requirement | Minimum | Notes |
|---|---|---|
| macOS | 13 Ventura | Metal renderer |
| Xcode Command Line Tools | 15+ | `xcode-select --install` |
| Apple Clang | 21+ (Xcode 16+) | C++23 `std::expected`, `std::format` |
| CMake | 3.25+ | `brew install cmake` |
| HDF5 | 1.8+ | `brew install hdf5` ← **only external dep** |

HDF5 is the only thing you need to install manually.  All other libraries
(Dear ImGui, ImPlot, GLFW, spdlog, nlohmann-json, Catch2) are fetched and
compiled automatically by CMake on first build.

### Linux (not yet tested)

The library layer (`gnc_viz_lib`) is platform-agnostic.  The app layer uses
Metal + NSOpenPanel (macOS only) and would need an OpenGL/Vulkan backend and a
portable file-dialog replacement.

---

## Build

### Quick start

```bash
# Install the only external system dependency
brew install hdf5

# Clone (or enter) the project
cd /path/to/curly-giggle

# Build (Debug, runs all tests afterwards)
chmod +x build.sh
./build.sh
```

### Build script options

```bash
./build.sh              # Debug build, runs ctest afterwards
./build.sh --release    # Optimised release build
./build.sh --clean      # Wipe build/ and rebuild from scratch
./build.sh --no-tests   # Skip ctest (faster iteration)
```

### Manual CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build
```

The binary is at `build/bin/gnc_viz`.  Tests are at `build/bin/gnc_viz_tests`.

---

## Run

```bash
# Launch the visualizer
./build/bin/gnc_viz

# Run the unit-test suite
./build/bin/gnc_viz_tests

# Or via ctest (with verbose output)
ctest --test-dir build -V
```

---

## How to use

### Opening files

- **Menu → File → Open…** (or **Cmd+O**) — opens a native file picker; supports `.h5` and `.hdf5`
- **Drag and drop** — drag one or more `.h5` files from Finder onto the window

Each loaded file appears in the **Files pane** (left panel).

### Browsing signals

The **Signals pane** (middle panel) shows a collapsible tree of every HDF5
dataset in the selected simulation file, with the full HDF5 path.

- Type in the **search box** at the top to filter signals by name
- Click **[+]** next to a signal to add it to the plot
- Vector signals (e.g. quaternions) expand to show individual components;
  click a component's **[+]** to plot only that component, or click the
  parent's **[+]** to plot all at once

### Plotting signals

Signals appear as coloured lines in the **Plot pane** (right panel).

| Action | How |
|---|---|
| Show/hide a signal | Toggle the checkbox next to its name |
| Rename (alias) | Double-click the signal name in the plotted-signal list |
| Change Y-axis | Right-click the signal name → Left (Y1) / Right (Y2) / Y3 |
| Configure axis range | Right-click the axis tick area in the plot |
| Re-zoom | **Fit All** button (both axes) or **Fit X** (X axis only) |
| Remove | Click the **✕** button next to the signal |

### Plot types

Select the active plot type from the toolbar at the top of the Plot pane:
`timeseries` · `trajectory2d` · `groundtrack`

### Tools

Click **annotation** or **ruler** in the toolbar to activate:
- **annotation** — click on the plot to drop a timestamped label
- **ruler** — click + drag to measure distance between two points

Click the same button again to deactivate the tool.

### Derived signals

With at least one signal plotted, click **+ Derived** to open the derived-signal dialog.
Choose an operation (add, subtract, scale, magnitude), select input signals, enter a name, and click **Create**.

### Exporting

- **File → Export CSV…** — saves all plotted signals to a time-aligned CSV file
- **File → Export PNG…** — saves a screenshot of the current window

### Session save / restore

- **File → Save Session** (or **Cmd+S**) — saves to `~/.gnc_viz/session.json`
- **File → Load Session…** — loads a JSON session from a file picker
- The last session is restored automatically on startup

### Pane layout

- **View → Files pane** and **View → Signals pane** toggle the side panels
- Drag the vertical splitter bars to resize panes
- With both panes hidden the plot fills the entire window

---

## Keyboard shortcuts

| Shortcut | Action |
|---|---|
| `Cmd+O` | Open HDF5 file |
| `Cmd+S` | Save session |
| `Cmd+Q` | Quit |

---

## Project structure

```
curly-giggle/
├── include/gnc_viz/          Public headers (data layer + interfaces)
│   ├── error.hpp             gnc::Result<T> / gnc::VoidResult (std::expected)
│   ├── interfaces.hpp        IPlotType, ISignalOperation, IVisualizationTool
│   ├── registry.hpp          Registry<Interface> plugin system
│   ├── signal_buffer.hpp     In-memory signal storage (time + values)
│   ├── signal_metadata.hpp   Metadata struct (name, units, shape, sim_id, …)
│   ├── signal_ops.hpp        Built-in ops: Add, Subtract, Multiply, Scale, Magnitude
│   ├── hdf5_reader.hpp       HDF5 enumerate + load
│   ├── color_manager.hpp     Per-signal color assignment
│   ├── config.hpp            Persistent config (JSON)
│   ├── app_state.hpp         Central UI state struct
│   └── …
├── src/
│   ├── lib/                  gnc_viz_lib — pure C++, no UI, fully unit-testable
│   ├── app/                  gnc_viz app — ImGui + Metal + ObjC++
│   └── tests/                Catch2 test suite
├── docs/
│   └── ARCHITECTURE.md       Module diagram, key classes, extension guide
├── cmake/
│   ├── deps.cmake            FetchContent declarations
│   └── compiler_flags.cmake  C++23 + warning flags
└── build.sh                  Convenience build script
```

---

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full module diagram,
key class descriptions, data-flow walkthrough, and extension guides.

---

## Tech stack

| Layer | Library | Version |
|---|---|---|
| GUI | Dear ImGui (docking branch) | latest |
| Plotting | ImPlot | latest |
| Windowing | GLFW | master |
| Renderer | Apple Metal | macOS system |
| HDF5 I/O | HDF5 C API | 1.14.6 |
| Logging | spdlog | latest |
| JSON | nlohmann-json | latest |
| Testing | Catch2 | v3 |
| Build | CMake | 3.25+ |

---

## Roadmap

- **Detachable plot windows** — tear off a plot into its own floating window
- **Sidereal-corrected ground track** — display ECEF position with Earth rotation
- **Derived signals UI** — visual signal-graph editor (node + wire)
- **Computed norms** — one-click ‖v‖ for any vector signal
- **Linux support** — OpenGL backend + portal file dialog

