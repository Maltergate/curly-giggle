#include "gnc_viz/trajectory_2d_plot.hpp"
#include "gnc_viz/app_state.hpp"
#include "implot.h"
#include "imgui.h"

namespace gnc_viz {

void Trajectory2DPlot::render(AppState& /*state*/, float width, float height) {
    const ImVec2 sz(width > 0 ? width : -1.0f, height > 0 ? height : -1.0f);
    if (ImPlot::BeginPlot("##trajectory2d", sz, ImPlotFlags_NoTitle)) {
        ImPlot::SetupAxes("X", "Y");
        // TODO Phase 2: render XY/YZ/XZ trajectory lines from vector signals
        ImPlot::EndPlot();
    }
    ImGui::TextDisabled("2D Trajectory — select plot axes from signal context menu (Phase 2)");
}

} // namespace gnc_viz
