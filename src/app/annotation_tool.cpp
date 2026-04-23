// annotation_tool.cpp — AnnotationTool implementation

#include "gnc_viz/annotation_tool.hpp"
#include "gnc_viz/app_state.hpp"

#include "imgui.h"
#include "implot.h"

#include <cstring>

namespace gnc_viz {

void AnnotationTool::handle_input(AppState& /*state*/)
{
    if (!ImPlot::IsPlotHovered()) return;

    // Right-click near an existing annotation → delete it immediately
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        const ImVec2 mouse = ImGui::GetMousePos();
        for (int i = static_cast<int>(m_annotations.size()) - 1; i >= 0; --i) {
            const ImVec2 px = ImPlot::PlotToPixels(m_annotations[i].time,
                                                   m_annotations[i].value);
            const float dx = mouse.x - px.x;
            const float dy = mouse.y - px.y;
            if (dx * dx + dy * dy <= 100.0f) {   // within 10 px radius
                m_annotations.erase(m_annotations.begin() + i);
                return;
            }
        }
    }

    // Left-click → open text-entry popup
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImPlotPoint mp = ImPlot::GetPlotMousePos();
        m_place_time  = mp.x;
        m_place_value = mp.y;
        m_placing     = true;
        std::memset(m_text_buf, 0, sizeof(m_text_buf));
        ImGui::OpenPopup("##anno_text");
    }

    // Text-entry popup (rendered in same ImGui window context)
    if (ImGui::BeginPopup("##anno_text")) {
        ImGui::Text("Annotation text:");
        ImGui::SetNextItemWidth(200.0f);
        const bool enter_pressed =
            ImGui::InputText("##anno_input", m_text_buf, sizeof(m_text_buf),
                             ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        const bool add_clicked = ImGui::Button("Add");

        if (enter_pressed || add_clicked) {
            m_annotations.push_back({m_place_time, m_place_value,
                                     std::string(m_text_buf), true});
            m_placing = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Cancel")) {
            m_placing = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void AnnotationTool::render_overlay(const AppState& /*state*/)
{
    ImDrawList* dl = ImPlot::GetPlotDrawList();
    if (!dl) return;

    for (const auto& a : m_annotations) {
        const ImVec2 px = ImPlot::PlotToPixels(a.time, a.value);

        // Circle marker
        dl->AddCircleFilled(px, 5.0f, IM_COL32(255, 200, 50, 200));

        // Text label above and to the right
        dl->AddText(ImVec2(px.x + 6.0f, px.y - 14.0f),
                    IM_COL32(255, 255, 255, 220),
                    a.text.c_str());

        // Arrow line from above to the marker
        if (a.has_arrow) {
            const ImVec2 arrow_start(px.x, px.y - 20.0f);
            dl->AddLine(arrow_start, px, IM_COL32(255, 200, 50, 200), 1.5f);
        }
    }
}

const std::vector<Annotation>& AnnotationTool::annotations() const noexcept
{
    return m_annotations;
}

void AnnotationTool::clear_annotations()
{
    m_annotations.clear();
}

} // namespace gnc_viz
