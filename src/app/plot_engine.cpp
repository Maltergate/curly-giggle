#include "gnc_viz/plot_engine.hpp"
#include "gnc_viz/timeseries_plot.hpp"
#include "gnc_viz/trajectory_2d_plot.hpp"
#include "gnc_viz/ground_track_plot.hpp"
#include "gnc_viz/log.hpp"
#include "gnc_viz/app_state.hpp"

namespace gnc_viz {

PlotEngine::PlotEngine() {
    m_registry.register_type<TimeSeriesPlot>("timeseries");
    m_registry.register_type<Trajectory2DPlot>("trajectory2d");
    m_registry.register_type<GroundTrackPlot>("groundtrack");

    // Activate timeseries by default
    m_current_id = "timeseries";
    m_current = m_registry.create("timeseries");
}

void PlotEngine::render(AppState& state, float width, float height) {
    if (m_current)
        m_current->render(state, width, height);
}

void PlotEngine::switch_to(std::string_view type_id, AppState& state) {
    if (type_id == m_current_id) return;
    if (!m_registry.contains(type_id)) {
        GNC_LOG_WARN("PlotEngine::switch_to: unknown type '{}'", type_id);
        return;
    }
    if (m_current) m_current->on_deactivate();
    m_current_id = std::string(type_id);
    m_current = m_registry.create(type_id);
    if (m_current) m_current->on_activate(state);
}

std::string_view PlotEngine::current_type_id() const noexcept { return m_current_id; }
const std::vector<std::string>& PlotEngine::available_types() const noexcept {
    return m_registry.keys();
}

void PlotEngine::fit_to_data() {
    if (auto* ts = dynamic_cast<TimeSeriesPlot*>(m_current.get()))
        ts->fit_to_data();
}
void PlotEngine::fit_x_only() {
    if (auto* ts = dynamic_cast<TimeSeriesPlot*>(m_current.get()))
        ts->fit_x_only();
}
void PlotEngine::fit_y_only() {
    if (auto* ts = dynamic_cast<TimeSeriesPlot*>(m_current.get()))
        ts->fit_y_only();
}

IPlotType*       PlotEngine::current_type() noexcept       { return m_current.get(); }
const IPlotType* PlotEngine::current_type() const noexcept { return m_current.get(); }

} // namespace gnc_viz
