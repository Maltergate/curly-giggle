#include "fastscope/trajectory_2d_plot.hpp"
#include "fastscope/app_state.hpp"
#include "fastscope/plotted_signal.hpp"
#include "implot.h"
#include "imgui.h"
#include <algorithm>

namespace fastscope {

// Convert packed 0xRRGGBBAA → ImVec4 in [0,1].
static ImVec4 unpack_rgba_traj(uint32_t rgba) noexcept
{
    return {
        ((rgba >> 24) & 0xFF) / 255.0f,
        ((rgba >> 16) & 0xFF) / 255.0f,
        ((rgba >>  8) & 0xFF) / 255.0f,
        ((rgba >>  0) & 0xFF) / 255.0f,
    };
}

// Populate (or refresh) component_cache for a vector signal.
static void ensure_component_cache(PlottedSignal& ps)
{
    if (ps.cache_source.lock() == ps.buffer) return;  // still valid
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
            float* dst = ps.component_cache[k].data();
            for (std::size_t i = 0; i < N; ++i)
                dst[i] = static_cast<float>(v[i * K + k]);
        }
    }
    ps.cache_source = ps.buffer;
}

void Trajectory2DPlot::render(AppState& state, float width, float height)
{
    // ── Axes selector toolbar ─────────────────────────────────────────────────
    ImGui::TextDisabled("Axes:");
    ImGui::SameLine();
    if (ImGui::RadioButton("X-Y", m_axes == Trajectory2DAxes::XY))
        m_axes = Trajectory2DAxes::XY;
    ImGui::SameLine();
    if (ImGui::RadioButton("Y-Z", m_axes == Trajectory2DAxes::YZ))
        m_axes = Trajectory2DAxes::YZ;
    ImGui::SameLine();
    if (ImGui::RadioButton("X-Z", m_axes == Trajectory2DAxes::XZ))
        m_axes = Trajectory2DAxes::XZ;

    // Map enum to component indices and axis labels
    int c_x = 0, c_y = 1;
    const char* x_label = "X";
    const char* y_label = "Y";
    if (m_axes == Trajectory2DAxes::YZ) { c_x = 1; c_y = 2; x_label = "Y"; y_label = "Z"; }
    if (m_axes == Trajectory2DAxes::XZ) { c_x = 0; c_y = 2; x_label = "X"; y_label = "Z"; }

    // ── Build/refresh component caches ───────────────────────────────────────
    for (auto& ps : state.plotted_signals) {
        if (!ps.buffer || !ps.buffer->is_loaded()) continue;
        if (ps.buffer->n_components() > 1)
            ensure_component_cache(ps);
    }

    // ── Begin plot ────────────────────────────────────────────────────────────
    const float toolbar_h = ImGui::GetFrameHeightWithSpacing();
    const ImVec2 plot_sz(
        width  > 0 ? width : -1.0f,
        (height > toolbar_h) ? height - toolbar_h : -1.0f);

    if (!ImPlot::BeginPlot("##traj2d", plot_sz,
                            ImPlotFlags_NoTitle | ImPlotFlags_Equal))
        return;

    const ImPlotAxisFlags af = m_fit_on_next ? ImPlotAxisFlags_AutoFit
                                              : ImPlotAxisFlags_None;
    ImPlot::SetupAxes(x_label, y_label, af, af);
    ImPlot::SetupLegend(ImPlotLocation_NorthEast);
    m_fit_on_next = false;

    // ── Render each visible vector signal ─────────────────────────────────────
    for (const auto& ps : state.plotted_signals) {
        if (!ps.visible || !ps.buffer || !ps.buffer->is_loaded()) continue;
        const auto& buf = *ps.buffer;
        const std::size_t N = buf.sample_count();
        const std::size_t K = buf.n_components();
        if (N == 0 || K < 2) continue;
        if (static_cast<std::size_t>(std::max(c_x, c_y)) >= K) continue;

        ImPlotSpec spec;
        spec.LineColor = unpack_rgba_traj(ps.color_rgba);

        const float* xs = ps.component_cache[static_cast<std::size_t>(c_x)].data();
        const float* ys = ps.component_cache[static_cast<std::size_t>(c_y)].data();
        ImPlot::PlotLine(ps.display_name().c_str(), xs, ys,
                         static_cast<int>(N), spec);
    }

    ImPlot::EndPlot();
}

} // namespace fastscope
