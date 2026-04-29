#pragma once
/// @file app_state.hpp
/// @brief Central application data model — AppState and sub-structs.
/// @defgroup app_layer Application Layer
/// @brief Top-level application state and lifecycle.

#include "fastscope/annotation_tool.hpp"  // Annotation struct
#include "fastscope/axis_manager.hpp"
#include "fastscope/color_manager.hpp"
#include "fastscope/plotted_signal.hpp"
#include "fastscope/simulation_file.hpp"
#include "fastscope/tool_manager.hpp"

#include <cstdint>
#include <memory>
#include <regex>
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

    /// Whether the floating log window is shown.
    bool log_pane_visible = false;
};

// ── Demo / debug toggles ───────────────────────────────────────────────────────

/// @brief Debug and demo window visibility flags.
struct DebugState {
    bool show_imgui_demo  = false;
    bool show_implot_demo = false;
    bool show_metrics     = false;
};

// ── Signal exclusion rules ─────────────────────────────────────────────────────

/// @brief A compiled ECMAScript regex rule that hides matching signals from the
///        signal tree.  Rules are global (apply to all loaded simulations).
struct SignalExclusionRule {
    std::string pattern;   ///< Raw regex string as entered by the user.
    std::regex  compiled;  ///< Compiled form — rebuilt once on rule add.
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

    // ── Annotations (owned here so they can be serialised independently of which
    //    tool is currently active) ──────────────────────────────────────────────
    std::vector<Annotation> annotations;

    // ── Tool system ───────────────────────────────────────────────────────────
    ToolManager tool_manager;

    // ── Color management ──────────────────────────────────────────────────────
    ColorManager colors;

    // ── Signal exclusion rules ────────────────────────────────────────────────
    /// Global regex rules that hide matching signals from the signal tree.
    /// Adding or removing a rule increments signal_exclusion_version so the
    /// per-simulation exclusion caches in the widget know to rebuild.
    std::vector<SignalExclusionRule> signal_exclusions;
    uint32_t signal_exclusion_version = 0;

    // ── UI / layout ───────────────────────────────────────────────────────────
    PaneState  panes;
    DebugState debug;

    // ── Application lifecycle ─────────────────────────────────────────────────
    bool quit_requested = false;
};

} // namespace fastscope
