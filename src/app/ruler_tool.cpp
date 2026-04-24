// ruler_tool.cpp — RulerTool implementation

#include "fastscope/ruler_tool.hpp"
#include "fastscope/app_state.hpp"

#include "imgui.h"
#include "implot.h"

#include <algorithm>
#include <cstdio>

namespace fastscope {

void RulerTool::handle_input(AppState& /*state*/)
{
    if (!ImPlot::IsPlotHovered()) return;

    const ImPlotPoint mp = ImPlot::GetPlotMousePos();

    // Track live cursor as tentative end position
    if (!m_has_end) {
        m_end_t = mp.x;
        m_end_v = mp.y;
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!m_has_start) {
            m_start_t = mp.x;
            m_start_v = mp.y;
            m_has_start = true;
        } else if (!m_has_end) {
            m_end_t = mp.x;
            m_end_v = mp.y;
            m_has_end = true;
        }
    }

    // Right-click resets the measurement
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_has_start = false;
        m_has_end   = false;
    }
}

void RulerTool::render_overlay(const AppState& /*state*/)
{
    ImDrawList* dl = ImPlot::GetPlotDrawList();
    if (!dl) return;

    constexpr ImU32 kColor = IM_COL32(100, 200, 255, 200);
    const ImPlotRect lim   = ImPlot::GetPlotLimits();
    const bool hovered     = ImPlot::IsPlotHovered();

    if (m_has_start && m_has_end) {
        // Horizontal line at the midpoint value
        const double mid_v = (m_start_v + m_end_v) * 0.5;
        const ImVec2 h0 = ImPlot::PlotToPixels(m_start_t, mid_v);
        const ImVec2 h1 = ImPlot::PlotToPixels(m_end_t,   mid_v);
        dl->AddLine(h0, h1, kColor, 1.5f);

        // Vertical line at start
        dl->AddLine(ImPlot::PlotToPixels(m_start_t, lim.Min().y),
                    ImPlot::PlotToPixels(m_start_t, lim.Max().y),
                    kColor, 1.0f);

        // Vertical line at end
        dl->AddLine(ImPlot::PlotToPixels(m_end_t, lim.Min().y),
                    ImPlot::PlotToPixels(m_end_t, lim.Max().y),
                    kColor, 1.0f);

        // Endpoint dots
        dl->AddCircleFilled(ImPlot::PlotToPixels(m_start_t, m_start_v), 4.0f, kColor);
        dl->AddCircleFilled(ImPlot::PlotToPixels(m_end_t,   m_end_v),   4.0f, kColor);

        // Measurement label at midpoint
        char buf[128];
        std::snprintf(buf, sizeof(buf),
                      "\xCE\x94T = %.4f  \xCE\x94V = %.4g",   // ΔT, ΔV (UTF-8)
                      m_end_t - m_start_t,
                      m_end_v - m_start_v);
        const ImVec2 mid_px = ImPlot::PlotToPixels((m_start_t + m_end_t) * 0.5, mid_v);
        dl->AddText(ImVec2(mid_px.x + 4.0f, mid_px.y - 16.0f), kColor, buf);

    } else if (m_has_start) {
        // Dashed vertical line at start
        const ImVec2 vs = ImPlot::PlotToPixels(m_start_t, lim.Min().y);
        const ImVec2 ve = ImPlot::PlotToPixels(m_start_t, lim.Max().y);

        constexpr float kDash = 6.0f, kGap = 4.0f;
        float cur_y = vs.y;
        while (cur_y < ve.y) {
            const float end_y = std::min(cur_y + kDash, ve.y);
            dl->AddLine(ImVec2(vs.x, cur_y), ImVec2(vs.x, end_y), kColor, 1.0f);
            cur_y += kDash + kGap;
        }

        // Crosshair dot at start
        dl->AddCircleFilled(ImPlot::PlotToPixels(m_start_t, m_start_v), 4.0f, kColor);
    }

    // Live cursor crosshair while hovered and awaiting second point
    if (hovered && m_has_start && !m_has_end) {
        const ImPlotPoint mp = ImPlot::GetPlotMousePos();
        const ImVec2 cpx = ImPlot::PlotToPixels(mp.x, mp.y);
        dl->AddLine(ImVec2(cpx.x - 8.0f, cpx.y), ImVec2(cpx.x + 8.0f, cpx.y), kColor, 1.0f);
        dl->AddLine(ImVec2(cpx.x, cpx.y - 8.0f), ImVec2(cpx.x, cpx.y + 8.0f), kColor, 1.0f);
    }
}

} // namespace fastscope
