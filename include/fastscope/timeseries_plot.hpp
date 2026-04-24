#pragma once
/// @file timeseries_plot.hpp
/// @brief IPlotType implementation for multi-Y-axis time-series line plots.
/// @ingroup plot_system

#include "fastscope/interfaces.hpp"

#include <string>
#include <utility>  // std::pair
#include <vector>

namespace fastscope {

/// @brief IPlotType implementation for multi-Y-axis time-series line plots.
/// @details Renders all PlottedSignals as colored lines using ImPlot.
///          Supports up to 3 Y-axes via AxisManager and auto-fit controls.
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

} // namespace fastscope
