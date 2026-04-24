# Prompt: Fix a Bug in FastScope

Use this prompt when you need to investigate and fix a bug. Follow this structured workflow.

---

## Context

FastScope is a C++23 + Dear ImGui + Metal macOS app at `/Users/thomasbarbier/Workspace/curly-giggle`.

- Build: `cmake --build build -j$(sysctl -n hw.ncpu)`
- Run: `./build/bin/fastscope`
- Tests: `./build/bin/fastscope_tests`
- Logs: `~/.fastscope/fastscope.log` (spdlog rotating file sink)

---

## Step 1 — Establish a clean baseline

```bash
cd /Users/thomasbarbier/Workspace/curly-giggle
git status          # confirm working tree is clean
cmake --build build -j$(sysctl -n hw.ncpu)
./build/bin/fastscope_tests    # all tests must pass before you start
```

If tests are already failing before your change, **stop and report** — this is a pre-existing issue.

---

## Step 2 — Reproduce the bug

Describe (or read from the issue) the exact steps to reproduce:
1. What file was loaded?
2. What signals were plotted?
3. What action triggered the bug?
4. What was expected vs what happened?

Check the log first:
```bash
tail -100 ~/.fastscope/fastscope.log
```

---

## Step 3 — Locate the root cause

Key areas by symptom:

| Symptom | Look here |
|---------|-----------|
| FPS drops / frame hitches | `src/app/timeseries_plot.cpp` — render loop, `build_component_cache()` |
| Plot doesn't update after signal add/remove | `AppState::plotted_signals` mutations, `PlottedSignal::cache_source` |
| Fit / AutoFit doesn't work | `m_fit_on_next_frame` flags, culling bypass in `timeseries_plot.cpp` |
| Signal tree empty / wrong | `src/lib/hdf5_reader.cpp`, `SimulationFile::enumerate_signals()` |
| Crash on HDF5 load | `src/lib/hdf5_reader.cpp`, check `Result<T>` error propagation |
| UI layout broken | `src/app/application.mm` — pane splitter logic, `AppState.panes` |
| Color not assigned | `src/app/application.mm` — `ColorManager::assign()` call |
| Registry returns null | `include/fastscope/registry.hpp` — check `create()`, key spelling |

Add `FASTSCOPE_LOG_DEBUG("...")` calls to narrow down. Logs go to `~/.fastscope/fastscope.log`.

---

## Step 4 — Write a failing test (if possible)

Before fixing, add a unit test that reproduces the bug in `src/tests/`:

```bash
./build/bin/fastscope_tests --list-tests    # see existing tests
```

If the bug is in library code (`src/lib/`, `include/fastscope/`), it should be unit-testable.
If it's in rendering code (`src/app/`), it may require manual reproduction.

---

## Step 5 — Fix and verify

Make the minimal change. Then:

```bash
cmake --build build -j$(sysctl -n hw.ncpu)
./build/bin/fastscope_tests       # all tests must pass
./build/bin/fastscope             # manually verify the bug is gone
```

---

## Step 6 — Regression check

Run the full test suite and confirm no regressions:

```bash
./build/bin/fastscope_tests -r compact    # compact output, shows all failures
```

---

## Common Bug Patterns

### `std::expected` / `Result<T>` unhandled error
```cpp
// WRONG — silently drops error
auto result = reader.load_signal(meta);
auto buf = *result;   // UB if result is error

// CORRECT — propagate or handle
auto result = FASTSCOPE_TRY(reader.load_signal(meta));
```

### Float cache not rebuilt after signal reload
```cpp
// If SignalBuffer is reloaded (weak_ptr expired), call:
sig.build_component_cache();
```

### ImPlot state leak
If a `BeginPlot()` call is not always matched with `EndPlot()` (e.g. early return on error),
ImPlot internal state becomes corrupted. Pattern:
```cpp
if (ImPlot::BeginPlot(...)) {
    // ...even on error, always reach EndPlot...
    ImPlot::EndPlot();
}
```

### Fit flags cleared before use
In `timeseries_plot.cpp`, fit flags must be saved **before** clearing them:
```cpp
bool fitting = m_fit_on_next_frame || m_fit_x_on_next_frame || m_fit_y_on_next_frame;
m_fit_on_next_frame = m_fit_x_on_next_frame = m_fit_y_on_next_frame = false;
if (fitting) { /* bypass culling */ }
```

---

## Acceptance Criteria

Before submitting the fix:
1. `./build/bin/fastscope_tests` — all tests pass, including any new regression test
2. Bug is manually verified as fixed
3. No new compiler warnings introduced
4. The fix is minimal (no unrelated refactoring)
