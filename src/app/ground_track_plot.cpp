#include "gnc_viz/ground_track_plot.hpp"
#include "gnc_viz/app_state.hpp"
#include "implot.h"
#include "imgui.h"

namespace gnc_viz {

void GroundTrackPlot::render(AppState& /*state*/, float width, float height) {
    const ImVec2 sz(width > 0 ? width : -1.0f, height > 0 ? height : -1.0f);
    if (ImPlot::BeginPlot("##groundtrack", sz, ImPlotFlags_NoTitle | ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("Longitude [deg]", "Latitude [deg]");
        ImPlot::SetupAxisLimits(ImAxis_X1, -180.0, 180.0, ImPlotCond_Once);
        ImPlot::SetupAxisLimits(ImAxis_Y1,  -90.0,  90.0, ImPlotCond_Once);
        // TODO Phase 2: project ECI positions to lat/lon and render ground track
        ImPlot::EndPlot();
    }
    ImGui::TextDisabled("Ground Track — requires ECI position vector signal (Phase 2)");
}

} // namespace gnc_viz
