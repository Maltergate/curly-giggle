#pragma once
#include "gnc_viz/interfaces.hpp"
#include <string>

namespace gnc_viz {

enum class Trajectory2DAxes { XY = 0, YZ = 1, XZ = 2 };

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

} // namespace gnc_viz
