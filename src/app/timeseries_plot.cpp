// timeseries_plot.cpp — TimeSeriesPlot IPlotType implementation
// Depends on ImPlot; compiled as part of the gnc_viz app target.

#include "gnc_viz/timeseries_plot.hpp"
#include "gnc_viz/app_state.hpp"

#include "implot.h"

namespace gnc_viz {

void TimeSeriesPlot::on_activate(const AppState& /*state*/)
{
    m_fit_on_next_frame = true;
}

void TimeSeriesPlot::on_deactivate()
{
    // nothing to clean up yet
}

void TimeSeriesPlot::render(const AppState& /*state*/, float width, float height)
{
    // ── Placeholder: render an empty ImPlot ───────────────────────────────────
    //
    // Phase 8 will wire this to AppState::plotted_signals + SignalBuffers.
    // For now, the walking-skeleton just proves ImPlot renders without crash.

    if (ImPlot::BeginPlot("##timeseries",
                          ImVec2(width > 0 ? width : -1.0f,
                                 height > 0 ? height : -1.0f),
                          ImPlotFlags_NoTitle))
    {
        ImPlot::SetupAxes("Time (s)", "Value",
                          ImPlotAxisFlags_AutoFit,
                          ImPlotAxisFlags_AutoFit);

        // TODO Phase 8: iterate state.plotted_signals, call ImPlot::PlotLine()

        ImPlot::EndPlot();
    }
}

} // namespace gnc_viz
