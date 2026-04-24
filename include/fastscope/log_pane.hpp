#pragma once
/// @file log_pane.hpp
/// @brief Floating log window — renders recent application log entries in a
///        resizable, dockable ImGui window.
/// @ingroup app_layer

namespace fastscope {

/// @brief Render the floating log window.
/// @param open  Pointer to the visibility flag; the window's close button
///              sets it to false.  Pass nullptr to omit the close button.
void render_log_window(bool* open);

} // namespace fastscope
