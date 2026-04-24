#pragma once
/// @file app_state.hpp
/// @brief Central application data model — AppState and sub-structs.
/// @defgroup app_layer Application Layer
/// @brief Top-level application state and lifecycle.

#include "fastscope/axis_manager.hpp"
#include "fastscope/color_manager.hpp"
#include "fastscope/plotted_signal.hpp"
#include "fastscope/simulation_file.hpp"
#include "fastscope/tool_manager.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace fastscope {

// ── Pane / layout state ────────────────────────────────────────────────────────

/// @brief Visibility and width state for the collapsible side panes.
struct PaneState {
    bool  file_pane_visible   = true;
    bool  signal_pane_visible = true;
    float file_pane_width     = 280.0f;
    float signal_pane_width   = 300.0f;
};

// ── Demo / debug toggles ───────────────────────────────────────────────────────

/// @brief Debug and demo window visibility flags.
struct DebugState {
    bool show_imgui_demo  = false;
    bool show_implot_demo = false;
    bool show_metrics     = false;
};

// ── Central state struct ───────────────────────────────────────────────────────

/// @brief Central application data model — owns all runtime state.
/// @details A single AppState instance lives in Application. Every subsystem
///          receives a (const) reference — no globals, no singletons.
///          Subsystems may mutate only their own section.
struct AppState {
    // ── Data layer ─────────────────────────────────────────────────────────────
    std::vector<std::unique_ptr<SimulationFile>> simulations;

    // ── Plot layer ─────────────────────────────────────────────────────────────
    std::vector<PlottedSignal> plotted_signals;
    AxisManager axis_manager;

    // ── Tool system ───────────────────────────────────────────────────────────
    ToolManager tool_manager;

    // ── Color management ──────────────────────────────────────────────────────
    ColorManager colors;

    // ── UI / layout ───────────────────────────────────────────────────────────
    PaneState  panes;
    DebugState debug;

    // ── Application lifecycle ─────────────────────────────────────────────────
    bool quit_requested = false;
};

} // namespace fastscope
