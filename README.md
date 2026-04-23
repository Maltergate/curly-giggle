# GNC Simulation Data Visualizer

An interactive C++23 / Dear ImGui desktop application for visualizing HDF5 output files from
GNC (Guidance, Navigation & Control) spacecraft simulations.

## Prerequisites

```bash
brew install hdf5
```

GLFW and all other dependencies are fetched automatically via CMake FetchContent on first build.

## Build

```bash
chmod +x build.sh
./build.sh              # Debug build + tests
./build.sh --clean      # Clean rebuild
./build.sh --release    # Optimised release build
./build.sh --no-tests   # Skip ctest
```

The binary is placed at `build/bin/gnc_viz`.

## Project structure

```
src/lib/      — gnc_viz_lib: data layer, signal processing (no UI, fully unit-testable)
src/app/      — gnc_viz: Dear ImGui application
src/tests/    — Catch2 test suite
include/      — Public headers for gnc_viz_lib
cmake/        — deps.cmake (FetchContent), compiler_flags.cmake
```

## Tech stack

| Layer | Library |
|-------|---------|
| GUI | Dear ImGui (docking branch) + GLFW + Metal |
| Plotting | ImPlot |
| HDF5 I/O | HDF5 C API |
| Logging | spdlog |
| JSON | nlohmann-json |
| Testing | Catch2 v3 |
| Build | CMake 3.25+ |
