#pragma once
/// @file log_pane.hpp
/// @brief Horizontal log pane — renders recent application log entries in the UI.
/// @ingroup app_layer

namespace fastscope {

/// @brief Render the log pane into the current ImGui window / child.
/// @param width  Available pixel width.
/// @param height Available pixel height (including the header row).
void render_log_pane(float width, float height);

} // namespace fastscope
