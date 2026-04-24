## Summary

<!-- One or two sentences describing what this PR does and why. -->

## Type of change

- [ ] Bug fix
- [ ] New feature (plot type / signal operation / tool / UI)
- [ ] Refactor (no functional change)
- [ ] Documentation
- [ ] Performance improvement
- [ ] Other: <!-- describe -->

## Changes made

<!-- List the key files changed and what was done in each. -->

## How to test

<!-- Step-by-step instructions to manually verify the change. -->

1. Build: `cmake --build build -j$(sysctl -n hw.ncpu)`
2. <!-- specific steps -->

## Checklist

### Build & Tests
- [ ] `cmake --build build -j$(sysctl -n hw.ncpu)` — zero errors, zero new warnings
- [ ] `./build/bin/fastscope_tests` — all tests pass (no regressions)
- [ ] New logic has Catch2 unit tests in `src/tests/`

### Code quality
- [ ] No raw owning pointers (`new` / `delete`) — use `unique_ptr` / `shared_ptr`
- [ ] No exceptions in `src/lib/` — errors returned via `fastscope::Result<T>`
- [ ] No heap allocation inside `render()` or `RunFrame()` methods
- [ ] Float caches (`time_cache_f`, `component_cache`) used for rendering — not raw `double` data

### New extension points (if applicable)
- [ ] New `IPlotType` registered in `PlotEngine::PlotEngine()`
- [ ] New `ISignalOperation` registered in `create_operation_registry()`
- [ ] New `IVisualizationTool` registered in `ToolManager::ToolManager()`
- [ ] New `.cpp` file added to the correct `CMakeLists.txt`

### Documentation & style
- [ ] `///` Doxygen comments on all `public` members in `include/fastscope/` headers
- [ ] No mentions of "GNC", "spacecraft", "guidance", "navigation", "control" in user-facing text
- [ ] `docs/pages/` updated if the feature changes user-visible behaviour
- [ ] README updated if the feature changes build instructions or usage

### For performance changes
- [ ] FPS stays ≥ 60 with ≥ 4 signals plotted (verified in Debug build)
- [ ] macOS Instruments trace attached (if profiling was needed)
