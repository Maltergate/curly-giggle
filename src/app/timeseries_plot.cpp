// timeseries_plot.cpp — TimeSeriesPlot IPlotType implementation
// Depends on ImPlot; compiled as part of the gnc_viz app target.

#include "gnc_viz/timeseries_plot.hpp"
#include "gnc_viz/app_state.hpp"
#include "gnc_viz/simulation_file.hpp"
#include "gnc_viz/plotted_signal.hpp"
#include "gnc_viz/log.hpp"

#include "implot.h"

#include <string>
#include <vector>

namespace gnc_viz {

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Extract one component from an interleaved (row-major) buffer.
static std::vector<double>
extract_component(const SignalBuffer& buf, std::size_t comp)
{
    const std::size_t N = buf.sample_count();
    const std::size_t K = buf.n_components();
    std::vector<double> out(N);
    const double* v = buf.values().data();
    for (std::size_t i = 0; i < N; ++i)
        out[i] = v[i * K + comp];
    return out;
}

/// Convert packed 0xRRGGBBAA → ImVec4 (r,g,b,a) in [0,1].
static ImVec4 unpack_rgba(uint32_t rgba) noexcept
{
    return {
        ((rgba >> 24) & 0xFF) / 255.0f,
        ((rgba >> 16) & 0xFF) / 255.0f,
        ((rgba >>  8) & 0xFF) / 255.0f,
        ((rgba >>  0) & 0xFF) / 255.0f,
    };
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void TimeSeriesPlot::on_activate(AppState& /*state*/)
{
    m_fit_on_next_frame = true;
}

void TimeSeriesPlot::on_deactivate() {}

// ── Render ────────────────────────────────────────────────────────────────────

void TimeSeriesPlot::render(AppState& state, float width, float height)
{
    // Lazy-load buffers for any newly-added plotted signals
    for (auto& ps : state.plotted_signals) {
        if (ps.buffer && ps.buffer->is_loaded()) continue;

        for (auto& sim_ptr : state.simulations) {
            if (sim_ptr->sim_id() != ps.sim_id) continue;
            auto res = sim_ptr->load_signal(ps.meta);
            if (res)
                ps.buffer = *res;
            else
                GNC_LOG_WARN("TimeSeriesPlot: load_signal failed for '{}': {}",
                             ps.meta.h5_path, res.error().message);
            break;
        }
    }

    const ImPlotFlags flags = ImPlotFlags_NoTitle;
    if (!ImPlot::BeginPlot("##timeseries",
                           ImVec2(width  > 0 ? width  : -1.0f,
                                  height > 0 ? height : -1.0f),
                           flags))
        return;

    ImPlotAxisFlags x_flags = ImPlotAxisFlags_None;
    ImPlotAxisFlags y_flags = ImPlotAxisFlags_None;
    if (m_fit_on_next_frame) {
        x_flags |= ImPlotAxisFlags_AutoFit;
        y_flags |= ImPlotAxisFlags_AutoFit;
        m_fit_on_next_frame = false;
    }
    ImPlot::SetupAxes("Time", "Value", x_flags, y_flags);

    for (const auto& ps : state.plotted_signals) {
        if (!ps.visible || !ps.buffer || !ps.buffer->is_loaded()) continue;

        const auto& buf = *ps.buffer;
        const std::size_t N = buf.sample_count();
        if (N == 0) continue;

        const double* t = buf.time().data();
        ImPlotSpec spec;
        spec.LineColor = unpack_rgba(ps.color_rgba);

        if (ps.component_index >= 0) {
            const auto vals = extract_component(buf, static_cast<std::size_t>(ps.component_index));
            const std::string label = ps.display_name() + "[" +
                                      std::to_string(ps.component_index) + "]";
            ImPlot::PlotLine(label.c_str(), t, vals.data(), static_cast<int>(N), spec);
        } else if (buf.n_components() == 1) {
            ImPlot::PlotLine(ps.display_name().c_str(),
                             t, buf.values().data(), static_cast<int>(N), spec);
        } else {
            for (std::size_t comp = 0; comp < buf.n_components(); ++comp) {
                const auto vals = extract_component(buf, comp);
                const std::string label = ps.display_name() + "[" + std::to_string(comp) + "]";
                ImPlot::PlotLine(label.c_str(), t, vals.data(), static_cast<int>(N), spec);
            }
        }
    }

    ImPlot::EndPlot();
}

} // namespace gnc_viz
