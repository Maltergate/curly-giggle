#pragma once
#include "gnc_viz/interfaces.hpp"

namespace gnc_viz {

// Trajectory2DPlot — renders X-Y, Y-Z, or X-Z trajectory from vector signals.
// Phase 2 implementation. Currently shows a placeholder.
class Trajectory2DPlot : public IPlotType {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "2D Trajectory"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "trajectory2d"; }
    void render(AppState& state, float width, float height) override;
    void on_activate(AppState&) override {}
    void on_deactivate() override {}
};

} // namespace gnc_viz
