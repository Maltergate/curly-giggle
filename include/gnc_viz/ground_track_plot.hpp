#pragma once
#include "gnc_viz/interfaces.hpp"

namespace gnc_viz {

// GroundTrackPlot — renders Lat/Lon ground track from ECI position signals.
// Phase 2 implementation. Currently shows a placeholder.
class GroundTrackPlot : public IPlotType {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Ground Track"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "groundtrack"; }
    void render(AppState& state, float width, float height) override;
    void on_activate(AppState&) override {}
    void on_deactivate() override {}
};

} // namespace gnc_viz
