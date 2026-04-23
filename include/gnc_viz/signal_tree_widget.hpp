#pragma once

// ── SignalTreeWidget — middle pane: browse and add signals to the plot ─────────

namespace gnc_viz {
struct AppState;

/// Render the signal browser tree in the middle pane.
/// Must be called inside an ImGui child window / BeginChild block.
void render_signal_tree(AppState& state);

} // namespace gnc_viz
