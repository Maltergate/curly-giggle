#pragma once
/// @file trajectory_2d_plot.hpp
/// @brief IPlotType implementation for 2D trajectory scatter plots.
/// @ingroup plot_system
#include "fastscope/interfaces.hpp"
#include <string>

namespace fastscope {

/// @brief Axis pair selection for the 2D trajectory plot.
enum class Trajectory2DAxes { XY = 0, YZ = 1, XZ = 2 };

/// @brief IPlotType implementation for 2D trajectory scatter plots.
/// @details Plots component pairs (XY, YZ, or XZ) from multi-component signals.
class Trajectory2DPlot : public IPlotType {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "2D Trajectory"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "trajectory2d"; }
    void render(AppState& state, float width, float height) override;
    void on_activate(AppState&) override { m_fit_on_next = true; }
    void on_deactivate() override {}

private:
    Trajectory2DAxes m_axes      = Trajectory2DAxes::XY;
    bool             m_fit_on_next = true;
};

} // namespace fastscope
