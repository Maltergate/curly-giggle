#pragma once

// ── AppState — central application data model ─────────────────────────────────
//
// Owns all runtime state. A single AppState instance lives in Application.
// Every subsystem receives a (const) reference — no globals, no singletons.
//
// Fields are organised by phase.  Unimplemented phases leave forward-declared
// stubs commented out so the header compiles at all phases of development.

#include <vector>
#include <string>
#include <cstdint>

namespace gnc_viz {

// ── Forward declarations (types added as their phases are implemented) ─────────
// class SimulationFile;   // Phase 2
// struct PlottedSignal;   // Phase 8
// struct AxisConfig;      // Phase 8

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
// Pass by AppState& for mutation, const AppState& for read-only access.

struct AppState {
    // ── Phase 2: Data layer ────────────────────────────────────────────────────
    // std::vector<std::unique_ptr<SimulationFile>> simulations;

    // ── Phase 8: Plot layer ────────────────────────────────────────────────────
    // std::vector<PlottedSignal> plotted_signals;
    // std::vector<AxisConfig>    y_axes;

    // ── UI / layout ───────────────────────────────────────────────────────────
    PaneState  panes;
    DebugState debug;

    // ── Application lifecycle ─────────────────────────────────────────────────
    bool quit_requested = false;
};

} // namespace gnc_viz
