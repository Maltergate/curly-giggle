// annotation_tool.cpp — AnnotationTool implementation

#include "fastscope/annotation_tool.hpp"
#include "fastscope/app_state.hpp"

#include "imgui.h"
#include "implot.h"

#include <cmath>
#include <cstring>

namespace fastscope {

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Returns the point on the axis-aligned rectangle [rmin,rmax] where the line
/// from rect centre toward @p target exits the rectangle.
static ImVec2 rect_edge_toward(ImVec2 rmin, ImVec2 rmax,
                               ImVec2 centre, ImVec2 target)
{
    const float dx = target.x - centre.x;
    const float dy = target.y - centre.y;
    if (std::abs(dx) < 0.5f && std::abs(dy) < 0.5f) return centre;

    const float hw = (rmax.x - rmin.x) * 0.5f;
    const float hh = (rmax.y - rmin.y) * 0.5f;
    const float tx = (std::abs(dx) > 0.001f) ? hw / std::abs(dx) : 1e9f;
    const float ty = (std::abs(dy) > 0.001f) ? hh / std::abs(dy) : 1e9f;
    const float t  = std::min(tx, ty);
    return ImVec2(centre.x + dx * t, centre.y + dy * t);
}

/// Draws a filled arrowhead at @p tip pointing away from @p from.
static void draw_arrowhead(ImDrawList* dl, ImVec2 tip, ImVec2 from,
                           float size, ImU32 colour)
{
    float dx = tip.x - from.x;
    float dy = tip.y - from.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;
    dx /= len; dy /= len;
    const float wx = -dy * size * 0.40f;
    const float wy =  dx * size * 0.40f;
    const ImVec2 base(tip.x - dx * size, tip.y - dy * size);
    dl->AddTriangleFilled(tip,
                          ImVec2(base.x + wx, base.y + wy),
                          ImVec2(base.x - wx, base.y - wy),
                          colour);
}

// ── AnnotationTool ────────────────────────────────────────────────────────────

void AnnotationTool::handle_input(AppState& state)
{
    // Popup must be checked BEFORE the hovered-guard so it keeps rendering
    // even when the mouse has moved off the plot into the popup itself.
    if (ImGui::BeginPopup("##anno_text")) {
        ImGui::Text("Annotation:");
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        const bool confirmed =
            ImGui::InputText("##anno_input", m_text_buf, sizeof(m_text_buf),
                             ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("Add") || confirmed) {
            if (m_text_buf[0] != '\0') {
                state.annotations.push_back({m_place_time, m_place_value,
                                             std::string(m_text_buf)});
            }
            m_placing = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_placing = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (!ImPlot::IsPlotHovered()) return;

    // Right-click near an existing anchor → delete it.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        const ImVec2 mouse = ImGui::GetMousePos();
        for (int i = static_cast<int>(state.annotations.size()) - 1; i >= 0; --i) {
            const ImVec2 px = ImPlot::PlotToPixels(state.annotations[i].time,
                                                   state.annotations[i].value);
            const float dx = mouse.x - px.x;
            const float dy = mouse.y - px.y;
            if (dx * dx + dy * dy <= 100.0f) {
                state.annotations.erase(state.annotations.begin() + i);
                return;
            }
        }
    }

    // Left-click → open text-entry popup.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImPlotPoint mp = ImPlot::GetPlotMousePos();
        m_place_time  = mp.x;
        m_place_value = mp.y;
        m_placing     = true;
        std::memset(m_text_buf, 0, sizeof(m_text_buf));
        ImGui::OpenPopup("##anno_text");
    }
}

void AnnotationTool::render_overlay(const AppState& state)
{
    ImDrawList* dl = ImPlot::GetPlotDrawList();
    if (!dl) return;

    constexpr float  pad        = 9.0f;
    constexpr float  rounding   = 6.0f;
    constexpr float  arrow_size = 9.0f;
    constexpr float  line_thick = 1.5f;
    constexpr float  dot_radius = 4.0f;
    constexpr ImU32  k_bg       = IM_COL32( 30,  30,  38, 235);
    constexpr ImU32  k_border   = IM_COL32(255, 200,  50, 220);
    constexpr ImU32  k_text     = IM_COL32(255, 255, 255, 240);
    constexpr ImU32  k_arrow    = IM_COL32(255, 200,  50, 220);
    constexpr ImU32  k_shadow   = IM_COL32(  0,   0,   0,  80);

    for (const auto& a : state.annotations) {
        const ImVec2 anchor(ImPlot::PlotToPixels(a.time, a.value));
        const ImVec2 bc(anchor.x + a.label_offset_x,
                        anchor.y + a.label_offset_y);  // bubble centre in pixels

        const ImVec2 tsz  = ImGui::CalcTextSize(a.text.c_str());
        const ImVec2 bmin(bc.x - tsz.x * 0.5f - pad, bc.y - tsz.y * 0.5f - pad);
        const ImVec2 bmax(bc.x + tsz.x * 0.5f + pad, bc.y + tsz.y * 0.5f + pad);

        // Drop shadow
        dl->AddRectFilled(ImVec2(bmin.x + 3, bmin.y + 3),
                          ImVec2(bmax.x + 3, bmax.y + 3),
                          k_shadow, rounding);

        // Bubble background + border
        dl->AddRectFilled(bmin, bmax, k_bg,     rounding);
        dl->AddRect      (bmin, bmax, k_border, rounding, 0, 1.5f);

        // Text centred in bubble
        dl->AddText(ImVec2(bc.x - tsz.x * 0.5f, bc.y - tsz.y * 0.5f),
                    k_text, a.text.c_str());

        // Arrow: bubble edge → anchor point
        const ImVec2 edge = rect_edge_toward(bmin, bmax, bc, anchor);
        dl->AddLine(edge, anchor, k_arrow, line_thick);
        draw_arrowhead(dl, anchor, edge, arrow_size, k_arrow);

        // Anchor dot
        dl->AddCircleFilled(anchor, dot_radius, k_arrow);
        dl->AddCircle      (anchor, dot_radius, k_border, 0, 1.2f);
    }
}

} // namespace fastscope
