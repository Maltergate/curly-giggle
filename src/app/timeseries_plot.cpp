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

/// Populate (or refresh) the component_cache for a vector signal.
/// For scalars (n_components == 1), the cache is left empty — the render loop
/// reads buf.values().data() directly without any copy.
static void build_component_cache(PlottedSignal& ps)
{
    const auto& buf = *ps.buffer;
    const std::size_t K = buf.n_components();
    const std::size_t N = buf.sample_count();

    if (K <= 1) {
        ps.component_cache.clear();
    } else {
        ps.component_cache.resize(K);
        const double* v = buf.values().data();
        for (std::size_t k = 0; k < K; ++k) {
            ps.component_cache[k].resize(N);
            double* dst = ps.component_cache[k].data();
            for (std::size_t i = 0; i < N; ++i)
                dst[i] = v[i * K + k];
        }
    }
    ps.cache_source = ps.buffer;
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
    // ── Load buffers and build caches for new/stale signals ───────────────────
    for (auto& ps : state.plotted_signals) {
        // Load buffer if not yet loaded
        if (!ps.buffer || !ps.buffer->is_loaded()) {
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

        // Build/refresh component cache when buffer is fresh or changed
        if (ps.buffer && ps.buffer->is_loaded()) {
            if (ps.cache_source.lock() != ps.buffer)
                build_component_cache(ps);
        }
    }

    // ── Begin plot ────────────────────────────────────────────────────────────
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

    // ── Draw signals — zero allocations per frame ─────────────────────────────
    for (const auto& ps : state.plotted_signals) {
        if (!ps.visible || !ps.buffer || !ps.buffer->is_loaded()) continue;

        const auto& buf = *ps.buffer;
        const std::size_t N = buf.sample_count();
        if (N == 0) continue;

        const double* t = buf.time().data();
        ImPlotSpec spec;
        spec.LineColor = unpack_rgba(ps.color_rgba);

        if (ps.component_index >= 0) {
            // Single pinned component — use cached column (zero alloc)
            const std::size_t c = static_cast<std::size_t>(ps.component_index);
            const double* vals = ps.component_cache.size() > c
                                 ? ps.component_cache[c].data()
                                 : buf.values().data();
            const std::string label = ps.display_name() + "[" +
                                      std::to_string(ps.component_index) + "]";
            ImPlot::PlotLine(label.c_str(), t, vals, static_cast<int>(N), spec);

        } else if (buf.n_components() == 1) {
            // Scalar — direct pointer, no copy ever
            ImPlot::PlotLine(ps.display_name().c_str(),
                             t, buf.values().data(), static_cast<int>(N), spec);

        } else {
            // Vector — iterate cached columns, zero alloc
            for (std::size_t comp = 0; comp < buf.n_components(); ++comp) {
                const double* vals = ps.component_cache[comp].data();
                const std::string label = ps.display_name() + "[" +
                                          std::to_string(comp) + "]";
                ImPlot::PlotLine(label.c_str(), t, vals,
                                 static_cast<int>(N), spec);
            }
        }
    }

    ImPlot::EndPlot();
}

} // namespace gnc_viz
