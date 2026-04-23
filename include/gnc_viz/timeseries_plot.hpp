#pragma once

// ── TimeSeriesPlot — minimal IPlotType for time-series line rendering ─────────
//
// Renders all PlottedSignals from AppState as colored lines on a single ImPlot.
// This is the "walking-skeleton" plot type: proves the end-to-end data flow
// before multi-Y-axis, zoom, and other Phase 8+ features are added.
//
// Features (this phase):
//   - Single Y-axis
//   - One line per plotted signal, colored from AppState/ColorManager
//   - Basic hover tooltip (time + value)
//   - No axis labels, no legend formatting beyond signal alias
//
// TODO Phase 8: replace with TimeSeriesPlotFull (multi-Y-axis via AxisManager)

#include "gnc_viz/interfaces.hpp"

namespace gnc_viz {

class TimeSeriesPlot : public IPlotType {
public:
    [[nodiscard]] std::string_view name() const noexcept override
    { return "Time Series"; }

    [[nodiscard]] std::string_view id() const noexcept override
    { return "timeseries"; }

    void render(const AppState& state, float width, float height) override;
    void on_activate(const AppState& state) override;
    void on_deactivate() override;

private:
    bool m_fit_on_next_frame = true;
};

} // namespace gnc_viz
