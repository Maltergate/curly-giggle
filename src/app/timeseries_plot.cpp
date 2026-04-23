// timeseries_plot.cpp — TimeSeriesPlot IPlotType implementation (Phase 8/9)
// Depends on ImPlot; compiled as part of the gnc_viz app target.

#include "gnc_viz/timeseries_plot.hpp"
#include "gnc_viz/app_state.hpp"
#include "gnc_viz/simulation_file.hpp"
#include "gnc_viz/plotted_signal.hpp"
#include "gnc_viz/axis_manager.hpp"
#include "gnc_viz/color_manager.hpp"
#include "gnc_viz/log.hpp"
#include "gnc_viz/tool_manager.hpp"

#include "implot.h"
#include "imgui.h"

#include <algorithm>
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

/// Map ps.y_axis integer (0/1/2) to the corresponding ImAxis enum value.
static ImAxis to_imaxis(int y) noexcept
{
    if (y == 1) return ImAxis_Y2;
    if (y == 2) return ImAxis_Y3;
    return ImAxis_Y1;
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
        if (ps.buffer && ps.buffer->is_loaded()) {
            if (ps.cache_source.lock() != ps.buffer)
                build_component_cache(ps);
        }
    }

    // ── Sync AxisManager: only when signal list / y_axis assignments change ─────
    {
        bool changed = (state.plotted_signals.size() != m_prev_axis_fingerprint.size());
        if (!changed) {
            for (std::size_t i = 0; i < state.plotted_signals.size(); ++i) {
                const auto& ps = state.plotted_signals[i];
                if (m_prev_axis_fingerprint[i].first  != ps.plot_key() ||
                    m_prev_axis_fingerprint[i].second != ps.y_axis) {
                    changed = true;
                    break;
                }
            }
        }
        if (changed) {
            state.axis_manager.clear_assignments();
            m_prev_axis_fingerprint.resize(state.plotted_signals.size());
            for (std::size_t i = 0; i < state.plotted_signals.size(); ++i) {
                const auto& ps = state.plotted_signals[i];
                state.axis_manager.assign(ps.plot_key(), ps.y_axis);
                m_prev_axis_fingerprint[i] = {ps.plot_key(), ps.y_axis};
            }
        }
    }

    // ── Begin plot ────────────────────────────────────────────────────────────
    if (!ImPlot::BeginPlot("##timeseries",
                           ImVec2(width  > 0 ? width  : -1.0f,
                                  height > 0 ? height : -1.0f),
                           ImPlotFlags_NoTitle))
        return;

    // ── Axis setup ────────────────────────────────────────────────────────────
    ImPlotAxisFlags x_flags = ImPlotAxisFlags_None;
    if (m_fit_on_next_frame || m_fit_x_on_next_frame)
        x_flags |= ImPlotAxisFlags_AutoFit;

    ImPlot::SetupAxis(ImAxis_X1, "Time [s]", x_flags);
    ImPlot::SetupLegend(ImPlotLocation_NorthEast);

    for (const auto& cfg : state.axis_manager.active_axes()) {
        const ImAxis imaxis = to_imaxis(cfg.id);

        ImPlotAxisFlags yf = ImPlotAxisFlags_None;
        if (cfg.id != 0)
            yf = ImPlotAxisFlags_AuxDefault;
        if (m_fit_on_next_frame)
            yf |= ImPlotAxisFlags_AutoFit;

        const char* default_label = (cfg.id == 1) ? "Y2" : (cfg.id == 2) ? "Y3" : "Value";
        const char* label = cfg.label.empty() ? default_label : cfg.label.c_str();

        ImPlot::SetupAxis(imaxis, label, yf);

        if (!cfg.auto_range)
            ImPlot::SetupAxisLimits(imaxis, cfg.range_min, cfg.range_max, ImPlotCond_Always);
    }

    if (m_fit_on_next_frame)  m_fit_on_next_frame  = false;
    if (m_fit_x_on_next_frame) m_fit_x_on_next_frame = false;

    // ── Draw signals ──────────────────────────────────────────────────────────
    for (const auto& ps : state.plotted_signals) {
        if (!ps.visible || !ps.buffer || !ps.buffer->is_loaded()) continue;

        const auto& buf = *ps.buffer;
        const std::size_t N = buf.sample_count();
        if (N == 0) continue;

        const double* t = buf.time().data();
        ImPlotSpec spec;
        spec.LineColor = unpack_rgba(ps.color_rgba);

        ImPlot::SetAxes(ImAxis_X1, to_imaxis(ps.y_axis));

        if (ps.component_index >= 0) {
            const std::size_t c = static_cast<std::size_t>(ps.component_index);
            const double* vals = ps.component_cache.size() > c
                                 ? ps.component_cache[c].data()
                                 : buf.values().data();
            char label[512];
            std::snprintf(label, sizeof(label), "%s[%d]",
                          ps.display_name().c_str(), ps.component_index);
            ImPlot::PlotLine(label, t, vals, static_cast<int>(N), spec);

        } else if (buf.n_components() == 1) {
            ImPlot::PlotLine(ps.display_name().c_str(),
                             t, buf.values().data(), static_cast<int>(N), spec);

        } else {
            for (std::size_t comp = 0; comp < buf.n_components(); ++comp) {
                const double* vals = ps.component_cache[comp].data();
                char label[512];
                std::snprintf(label, sizeof(label), "%s[%zu]",
                              ps.display_name().c_str(), comp);
                ImPlot::PlotLine(label, t, vals,
                                 static_cast<int>(N), spec);
            }
        }
    }

    // ── Crosshair + interpolated-value tooltip ────────────────────────────────
    if (ImPlot::IsPlotHovered()) {
        const ImPlotPoint mp  = ImPlot::GetPlotMousePos();
        const ImPlotRect  lim = ImPlot::GetPlotLimits();
        ImDrawList*       dl  = ImPlot::GetPlotDrawList();

        // Vertical crosshair line
        const ImVec2 p0 = ImPlot::PlotToPixels(mp.x, lim.Min().y);
        const ImVec2 p1 = ImPlot::PlotToPixels(mp.x, lim.Max().y);
        dl->AddLine(p0, p1, IM_COL32(200, 200, 200, 100), 1.0f);

        // Tooltip showing time and interpolated signal values
        ImGui::BeginTooltip();
        ImGui::Text("t = %.4f", mp.x);
        ImGui::Separator();

        for (const auto& ps : state.plotted_signals) {
            if (!ps.visible || !ps.buffer || !ps.buffer->is_loaded()) continue;
            const auto t_span = ps.buffer->time();
            if (t_span.empty()) continue;

            auto it = std::lower_bound(t_span.begin(), t_span.end(), mp.x);
            std::size_t idx = (it == t_span.end())
                ? t_span.size() - 1
                : static_cast<std::size_t>(it - t_span.begin());

            if (idx > 0 && idx < t_span.size()) {
                const double t0    = t_span[idx - 1];
                const double t1    = t_span[idx];
                const double alpha = std::clamp(
                    (t1 > t0) ? (mp.x - t0) / (t1 - t0) : 0.0,
                    0.0, 1.0);

                float col[4];
                ColorManager::to_float4(ps.color_rgba, col);
                ImGui::TextColored(ImVec4(col[0], col[1], col[2], col[3]), "●");
                ImGui::SameLine();

                if (ps.component_index >= 0) {
                    const std::size_t c = static_cast<std::size_t>(ps.component_index);
                    const double v0 = ps.buffer->at(idx - 1, c);
                    const double v1 = ps.buffer->at(idx,     c);
                    ImGui::Text("%s[%d]: %.6g",
                                ps.display_name().c_str(),
                                ps.component_index,
                                v0 * (1.0 - alpha) + v1 * alpha);
                } else if (ps.buffer->n_components() == 1) {
                    const double v0 = ps.buffer->at(idx - 1);
                    const double v1 = ps.buffer->at(idx);
                    ImGui::Text("%s: %.6g",
                                ps.display_name().c_str(),
                                v0 * (1.0 - alpha) + v1 * alpha);
                } else {
                    ImGui::Text("%s:", ps.display_name().c_str());
                    for (std::size_t k = 0; k < ps.buffer->n_components(); ++k) {
                        const double v0 = ps.buffer->at(idx - 1, k);
                        const double v1 = ps.buffer->at(idx,     k);
                        ImGui::Text("  [%zu]: %.6g",
                                    k, v0 * (1.0 - alpha) + v1 * alpha);
                    }
                }
            }
        }
        ImGui::EndTooltip();
    }

    // ── Axis range right-click popups (must be inside BeginPlot block) ────────
    // ── Tool system tick ──────────────────────────────────────────────────────
    state.tool_manager.tick(state);

    static constexpr const char* k_popup_ids[3] = {
        "##ax_range_0", "##ax_range_1", "##ax_range_2"
    };

    for (int ax_idx = 0; ax_idx < 3; ++ax_idx) {
        if (ax_idx > 0 && !state.axis_manager.has_signals_on(ax_idx)) continue;
        const ImAxis imaxis = to_imaxis(ax_idx);
        if (ImPlot::IsAxisHovered(imaxis) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup(k_popup_ids[ax_idx]);
    }

    ImPlot::EndPlot();

    // Render axis range popups outside BeginPlot
    for (int ax_idx = 0; ax_idx < 3; ++ax_idx) {
        if (ax_idx > 0 && !state.axis_manager.has_signals_on(ax_idx)) continue;
        if (ImGui::BeginPopup(k_popup_ids[ax_idx])) {
            auto& cfg = state.axis_manager.axis_config(ax_idx);
            ImGui::TextDisabled("Y%d range", ax_idx + 1);
            ImGui::Separator();
            ImGui::Checkbox("Auto range##ax", &cfg.auto_range);
            if (!cfg.auto_range) {
                ImGui::SetNextItemWidth(140.0f);
                ImGui::InputDouble("Min##ax", &cfg.range_min, 0.0, 0.0, "%.4g");
                ImGui::SetNextItemWidth(140.0f);
                ImGui::InputDouble("Max##ax", &cfg.range_max, 0.0, 0.0, "%.4g");
            }
            ImGui::Separator();
            static char s_label_buf[64] = {};
            std::strncpy(s_label_buf, cfg.label.c_str(), sizeof(s_label_buf) - 1);
            s_label_buf[sizeof(s_label_buf) - 1] = '\0';
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::InputText("Label##ax", s_label_buf, sizeof(s_label_buf)))
                cfg.label = s_label_buf;
            ImGui::EndPopup();
        }
    }
}

} // namespace gnc_viz
