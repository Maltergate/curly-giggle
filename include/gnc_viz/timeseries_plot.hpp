#pragma once

// ── TimeSeriesPlot — Phase 8/9 IPlotType for time-series line rendering ───────
//
// Renders all PlottedSignals from AppState as colored lines on an ImPlot.
//
// Features (Phase 8/9):
//   - Multi-Y-axis rendering via AxisManager (Y1/Y2/Y3 with SetupAxis/SetAxes)
//   - Axis labels from AxisConfig.label; AuxDefault flags for secondary axes
//   - Fit All / Fit X buttons trigger ImPlotAxisFlags_AutoFit next frame
//   - Crosshair: vertical line + interpolated-value tooltip on hover
//   - Legend positioned NorthEast via SetupLegend
//
// Performance notes:
//   - component_cache: per-signal columnar cache built once on load, not every frame
//   - axis sync uses a dirty flag: AxisManager is only updated when the signal list
//     (plot_keys + y_axis assignments) changes, not on every frame

#include "gnc_viz/interfaces.hpp"

#include <string>
#include <utility>  // std::pair
#include <vector>

namespace gnc_viz {

class TimeSeriesPlot : public IPlotType {
public:
    [[nodiscard]] std::string_view name() const noexcept override
    { return "Time Series"; }

    [[nodiscard]] std::string_view id() const noexcept override
    { return "timeseries"; }

    void render(AppState& state, float width, float height) override;
    void on_activate(AppState& state) override;
    void on_deactivate() override;

    /// Trigger auto-fit on both X and all Y axes next frame.
    void fit_to_data()  { m_fit_on_next_frame  = true; }
    /// Trigger auto-fit on X axis only next frame.
    void fit_x_only()   { m_fit_x_on_next_frame = true; }
    /// Trigger auto-fit on all Y axes only next frame.
    void fit_y_only()   { m_fit_y_on_next_frame = true; }

private:
    bool m_fit_on_next_frame   = true;
    bool m_fit_x_on_next_frame = false;
    bool m_fit_y_on_next_frame = false;

    /// Cached fingerprint of the signal list: (plot_key, y_axis) for each signal.
    /// When this changes, AxisManager assignments are re-synced.
    std::vector<std::pair<std::string, int>> m_prev_axis_fingerprint;
};

} // namespace gnc_viz
