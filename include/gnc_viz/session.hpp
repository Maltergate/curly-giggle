#pragma once

// ── Session serialisation (stub) ──────────────────────────────────────────────
//
// Full implementation: Phase 14 (session-save-restore).
//
// The session JSON file stores the full AppState snapshot so the application
// can resume exactly where it left off.
//
// ── Planned JSON schema ────────────────────────────────────────────────────────
//
//  {
//    "version": 1,
//    "simulations": [
//      { "path": "/data/sim_2025_01.h5", "sim_id": "sim0" }
//    ],
//    "plotted_signals": [
//      {
//        "signal_key": "sim0//attitude/quaternion_body",
//        "alias": "q_body",
//        "color": "0x4477AAFF",
//        "y_axis_index": 0,
//        "visible": true
//      }
//    ],
//    "y_axes": [
//      { "label": "Quaternion", "min": -1.1, "max": 1.1, "auto_range": true }
//    ],
//    "zoom": { "x_min": 0.0, "x_max": 100.0 },
//    "active_plot_type": "timeseries",
//    "annotations": [],
//    "panes": {
//      "file_pane_visible": true,
//      "signal_pane_visible": true,
//      "file_pane_width": 280.0,
//      "signal_pane_width": 300.0
//    }
//  }

#include <filesystem>
#include <string>

namespace gnc_viz {

struct AppState;

/// Default session file path: ~/.gnc_viz/session.json
[[nodiscard]] std::filesystem::path default_session_path();

/// Save the current AppState to a session file.
/// Returns false and logs a warning on failure (non-fatal).
/// STUB: does nothing until Phase 14.
bool save_session(const AppState& state,
                  const std::filesystem::path& path = default_session_path());

/// Load AppState from a session file into an existing AppState.
/// Fields not present in the file are left at their defaults.
/// Returns false if the file doesn't exist or cannot be parsed.
/// STUB: does nothing until Phase 14.
bool load_session(AppState& state,
                  const std::filesystem::path& path = default_session_path());

} // namespace gnc_viz
