#include "fastscope/plot_engine.hpp"
#include "fastscope/timeseries_plot.hpp"
// trajectory_2d_plot.hpp and ground_track_plot.hpp are included here so they
// are compiled alongside the engine; register them below when the
// implementations are sufficiently polished.
#include "fastscope/trajectory_2d_plot.hpp"
#include "fastscope/ground_track_plot.hpp"
#include "fastscope/log.hpp"
#include "fastscope/app_state.hpp"

namespace fastscope {

PlotEngine::PlotEngine() {
    m_registry.register_type<TimeSeriesPlot>("timeseries");
    // Trajectory2DPlot and GroundTrackPlot: pending polish before re-enabling
    // m_registry.register_type<Trajectory2DPlot>("trajectory2d");
    // m_registry.register_type<GroundTrackPlot>("groundtrack");

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
        FASTSCOPE_LOG_WARN("PlotEngine::switch_to: unknown type '{}'", type_id);
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
    if (m_current) m_current->request_fit(/*x=*/true, /*y=*/true);
}
void PlotEngine::fit_x_only() {
    if (m_current) m_current->request_fit(/*x=*/true, /*y=*/false);
}
void PlotEngine::fit_y_only() {
    if (m_current) m_current->request_fit(/*x=*/false, /*y=*/true);
}

IPlotType*       PlotEngine::current_type() noexcept       { return m_current.get(); }
const IPlotType* PlotEngine::current_type() const noexcept { return m_current.get(); }

} // namespace fastscope
