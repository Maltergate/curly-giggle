# GNC Viz — Spacecraft Simulation Data Visualizer

An interactive **C++23 / Dear ImGui** desktop application for loading and comparing HDF5 output
files from GNC (Guidance, Navigation & Control) spacecraft simulations.

## What it does (roadmap)

- Load multiple `.h5` simulation result files side-by-side
- Browse every signal in a collapsible tree (datasets / groups from the HDF5 hierarchy)
- Plot signals as time-series, trajectory (2-D XY/XZ/YZ), or ground-track overlays
- Compare signals from different simulation runs on the same axes
- Multi-Y-axis support (e.g. quaternion + position on one plot)
- Compute derived signals: add, subtract, scale, vector magnitude, …
- Annotate plots and measure distances with a ruler tool
- Collapsible side panes to maximise plot area on a single screen
- Persistent config + session restore

> **Current status:** foundational infrastructure complete (HDF5 reader, signal buffers,
> signal operations, core UI layout, color manager, registry).  The walking-skeleton
> milestone (load a file → click signal → see line) is the next milestone.

---

## Requirements

### macOS (primary platform)

| Requirement | Minimum | Notes |
|---|---|---|
| macOS | 13 Ventura | Metal renderer |
| Xcode Command Line Tools | 15+ | `xcode-select --install` |
| Apple Clang | 21+ (Xcode 16+) | C++23 `std::expected`, `std::format` |
| CMake | 3.25+ | `brew install cmake` |
| HDF5 | 1.14+ | `brew install hdf5` ← **only manual dep** |

All other libraries (Dear ImGui, ImPlot, GLFW, spdlog, nlohmann-json, Catch2) are fetched
automatically via CMake `FetchContent` on first build — no extra `brew install` needed.

### Linux (not yet tested)

The library layer (`gnc_viz_lib`) is platform-agnostic.  The app layer uses Metal + NSOpenPanel
(macOS only) and would need an OpenGL/Vulkan backend and a portable file-dialog replacement.

---

## Build

### Quick start

```bash
# Install the only external system dependency
brew install hdf5

# Clone (or enter) the project
cd /path/to/curly-giggle

# Build (Debug + runs all tests)
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
│   └── tests/                Catch2 test suite (92 tests)
├── cmake/
│   ├── deps.cmake            FetchContent declarations
│   └── compiler_flags.cmake  C++23 + warning flags
└── build.sh                  Convenience build script
```

---

## Architecture

The codebase is split into two targets:

| Target | Language | Dependencies | Purpose |
|---|---|---|---|
| `gnc_viz_lib` | C++23 | HDF5, spdlog, nlohmann-json | Data layer — testable without a display |
| `gnc_viz` | C++23 + ObjC++ | ImGui, ImPlot, Metal, GLFW | UI application |

Extension points use pure-abstract interfaces (`IPlotType`, `ISignalOperation`,
`IVisualizationTool`) registered via `Registry<Interface>`.  New plot types and signal
operations can be added without touching existing code.

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
