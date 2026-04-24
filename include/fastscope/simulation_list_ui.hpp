#pragma once
/// @file simulation_list_ui.hpp
/// @brief Renders the simulation files list in the left pane.
/// @ingroup ui_widgets

namespace fastscope {
struct AppState;

/// @brief Render the "Simulations" left pane content.
/// @param state Application state (mutable so files can be loaded/unloaded).
void render_simulation_list(AppState& state);

} // namespace fastscope
