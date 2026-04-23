#pragma once
/// @file signal_tree_widget.hpp
/// @brief Renders the hierarchical signal browser in the middle pane.
/// @defgroup ui_widgets UI Widgets
/// @brief Reusable ImGui widget functions.

namespace gnc_viz {
struct AppState;

/// @brief Render the signal browser tree in the middle pane.
/// @param state Application state (mutable so signals can be added to the plot).
void render_signal_tree(AppState& state);

} // namespace gnc_viz
