#pragma once
// PlotEngine — owns the active IPlotType and PlotRegistry.
// Registers all built-in plot types on construction.
// application.mm uses PlotEngine instead of a bare TimeSeriesPlot.

#include "gnc_viz/registry.hpp"
#include "gnc_viz/interfaces.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gnc_viz {

struct AppState;

class PlotEngine {
public:
    PlotEngine();   // registers TimeSeriesPlot, Trajectory2DPlot, GroundTrackPlot

    // Render the active plot type
    void render(AppState& state, float width, float height);

    // Switch active plot type; calls on_deactivate/on_activate
    void switch_to(std::string_view type_id, AppState& state);

    [[nodiscard]] std::string_view current_type_id() const noexcept;
    [[nodiscard]] const std::vector<std::string>& available_types() const noexcept;

    // Convenience — delegated to active type if it's a TimeSeriesPlot
    void fit_to_data();
    void fit_x_only();

    // Direct access for type-specific UI
    IPlotType*       current_type() noexcept;
    const IPlotType* current_type() const noexcept;

private:
    PlotRegistry                 m_registry;
    std::string                  m_current_id;
    std::unique_ptr<IPlotType>   m_current;
};

} // namespace gnc_viz
