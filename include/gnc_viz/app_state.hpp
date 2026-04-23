#pragma once

// ── AppState — central application data model ─────────────────────────────────
//
// Owns all runtime state. A single AppState instance lives in Application.
// Every subsystem receives a (const) reference — no globals, no singletons.

#include "gnc_viz/axis_manager.hpp"
#include "gnc_viz/color_manager.hpp"
#include "gnc_viz/plotted_signal.hpp"
#include "gnc_viz/simulation_file.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gnc_viz {

// ── Pane / layout state ────────────────────────────────────────────────────────

struct PaneState {
    bool  file_pane_visible   = true;
    bool  signal_pane_visible = true;
    float file_pane_width     = 280.0f;
    float signal_pane_width   = 300.0f;
};

// ── Demo / debug toggles ───────────────────────────────────────────────────────

struct DebugState {
    bool show_imgui_demo  = false;
    bool show_implot_demo = false;
    bool show_metrics     = false;
};

// ── Central state struct ───────────────────────────────────────────────────────
//
// Rule: subsystems may mutate only their own section of AppState.
// Pass AppState& for mutation, const AppState& for read-only access.

struct AppState {
    // ── Data layer ─────────────────────────────────────────────────────────────
    std::vector<std::unique_ptr<SimulationFile>> simulations;

    // ── Plot layer ─────────────────────────────────────────────────────────────
    std::vector<PlottedSignal> plotted_signals;
    AxisManager axis_manager;

    // ── Color management ──────────────────────────────────────────────────────
    ColorManager colors;

    // ── UI / layout ───────────────────────────────────────────────────────────
    PaneState  panes;
    DebugState debug;

    // ── Application lifecycle ─────────────────────────────────────────────────
    bool quit_requested = false;
};

} // namespace gnc_viz
