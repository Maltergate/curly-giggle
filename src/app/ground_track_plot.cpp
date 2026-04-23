#include "gnc_viz/ground_track_plot.hpp"
#include "gnc_viz/app_state.hpp"
#include "gnc_viz/plotted_signal.hpp"
#include "gnc_viz/signal_buffer.hpp"
#include "implot.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace gnc_viz {

static constexpr double RAD_TO_DEG = 180.0 / M_PI;

// Convert packed 0xRRGGBBAA → ImVec4 in [0,1].
static ImVec4 unpack_rgba_gt(uint32_t rgba) noexcept
{
    return {
        ((rgba >> 24) & 0xFF) / 255.0f,
        ((rgba >> 16) & 0xFF) / 255.0f,
        ((rgba >>  8) & 0xFF) / 255.0f,
        ((rgba >>  0) & 0xFF) / 255.0f,
    };
}

// static member — can name GroundTrackPlot::GroundTrackCache freely here.
void GroundTrackPlot::build_ground_track(GroundTrackCache& cache,
                                          const SignalBuffer& buf)
{
    const std::size_t N = buf.sample_count();
    const std::size_t K = buf.n_components();
    if (K < 3 || N == 0) { cache.lats.clear(); cache.lons.clear(); return; }

    cache.lats.resize(N);
    cache.lons.resize(N);
    const double* v = buf.values().data();

    for (std::size_t i = 0; i < N; ++i) {
        const double rx = v[i * K + 0];
        const double ry = v[i * K + 1];
        const double rz = v[i * K + 2];
        const double r  = std::sqrt(rx*rx + ry*ry + rz*rz);
        if (r < 1e-9) { cache.lats[i] = 0.0; cache.lons[i] = 0.0; continue; }
        // Geocentric latitude; inertial longitude (no Earth-rotation / sidereal correction)
        cache.lats[i] = std::asin(std::clamp(rz / r, -1.0, 1.0)) * RAD_TO_DEG;
        cache.lons[i] = std::atan2(ry, rx) * RAD_TO_DEG;
    }
}

void GroundTrackPlot::render(AppState& state, float width, float height)
{
    ImGui::TextDisabled("Note: inertial longitude (no Earth rotation correction)");

    const float note_h = ImGui::GetFrameHeightWithSpacing();
    const ImVec2 sz(
        width  > 0 ? width : -1.0f,
        (height > note_h) ? height - note_h : -1.0f);

    if (!ImPlot::BeginPlot("##groundtrack", sz,
                            ImPlotFlags_NoTitle | ImPlotFlags_Equal))
        return;

    ImPlot::SetupAxes("Longitude [deg]", "Latitude [deg]");
    ImPlot::SetupAxisLimits(ImAxis_X1, -180.0, 180.0, ImPlotCond_Once);
    ImPlot::SetupAxisLimits(ImAxis_Y1,  -90.0,  90.0, ImPlotCond_Once);
    ImPlot::SetupLegend(ImPlotLocation_NorthEast);

    for (const auto& ps : state.plotted_signals) {
        if (!ps.visible || !ps.buffer || !ps.buffer->is_loaded()) continue;
        const auto& buf = *ps.buffer;
        if (buf.n_components() < 3 || buf.sample_count() == 0) continue;

        const std::string key = ps.plot_key();
        auto& cache = m_cache[key];
        if (cache.source.lock() != ps.buffer) {
            build_ground_track(cache, buf);
            cache.source = ps.buffer;
        }
        if (cache.lats.empty()) continue;

        ImPlotSpec spec;
        spec.LineColor = unpack_rgba_gt(ps.color_rgba);
        ImPlot::PlotLine(ps.display_name().c_str(),
                         cache.lons.data(), cache.lats.data(),
                         static_cast<int>(cache.lats.size()), spec);
    }

    ImPlot::EndPlot();
}

} // namespace gnc_viz
