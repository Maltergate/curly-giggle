#pragma once

// ── SimulationListUI — left pane: load and manage simulation files ─────────────

namespace gnc_viz {
struct AppState;

/// Render the "Simulations" left pane content.
/// Must be called inside an ImGui child window / BeginChild block.
void render_simulation_list(AppState& state);

} // namespace gnc_viz
