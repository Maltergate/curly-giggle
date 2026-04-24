#include "fastscope/collapsible_pane.hpp"
#include "imgui.h"

namespace fastscope {

bool CollapsiblePane::begin(float width, float avail_h)
{
    if (!m_visible) return false;

    if (!*m_visible) {
        // ── Collapsed state: render a narrow strip with an expand button ───────
        m_last_width = kCollapsedWidth;
        ImGui::BeginChild(m_id.c_str(), ImVec2(kCollapsedWidth, avail_h),
                          ImGuiChildFlags_None);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.4f, 0.4f, 0.45f, 1.0f));

        // Expand button (▶)
        if (ImGui::Button("##expand", ImVec2(kCollapsedWidth, kCollapsedWidth)))
            *m_visible = true;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Expand %s pane", m_label.c_str());

        // Rotated label — draw via ImDrawList so we can rotate text
        {
            ImDrawList* dl   = ImGui::GetWindowDrawList();
            ImVec2      wpos = ImGui::GetWindowPos();
            // Draw a vertical "▶ <label>" indicator using a thin colored bar
            dl->AddRectFilled(
                ImVec2(wpos.x + kCollapsedWidth - 3.0f, wpos.y + kCollapsedWidth + 4.0f),
                ImVec2(wpos.x + kCollapsedWidth,        wpos.y + avail_h - 4.0f),
                IM_COL32(80, 80, 90, 180));
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
        ImGui::EndChild();
        return false;  // content must NOT be rendered
    }

    // ── Expanded state ────────────────────────────────────────────────────────
    m_last_width = width;
    ImGui::BeginChild(m_id.c_str(), ImVec2(width, avail_h), ImGuiChildFlags_None);

    // Header bar
    {
        ImDrawList* dl   = ImGui::GetWindowDrawList();
        ImVec2      wpos = ImGui::GetWindowPos();
        float       ww   = ImGui::GetWindowWidth();
        dl->AddRectFilled(wpos, ImVec2(wpos.x + ww, wpos.y + kHeaderHeight),
                          IM_COL32(45, 45, 52, 255));
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
    ImGui::TextUnformatted(m_label.c_str());
    ImGui::SameLine();

    // Collapse button (◀) pushed to the right
    float btn_x = ImGui::GetWindowWidth() - kCollapsedWidth - 4.0f;
    ImGui::SetCursorPosX(btn_x);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.4f, 0.4f, 0.45f, 1.0f));

    if (ImGui::Button("##collapse", ImVec2(kCollapsedWidth, kCollapsedWidth)))
        *m_visible = false;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Collapse %s pane", m_label.c_str());

    ImGui::PopStyleColor(3);

    ImGui::SetCursorPosY(kHeaderHeight + 2.0f);
    return true;
}

void CollapsiblePane::end()
{
    ImGui::EndChild();
}

} // namespace fastscope
