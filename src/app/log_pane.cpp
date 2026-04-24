// log_pane.cpp — Full-width horizontal pane showing recent application log entries.

#include "fastscope/log_pane.hpp"
#include "fastscope/ui_log_sink.hpp"

#include "imgui.h"

#include <string>
#include <vector>

namespace fastscope {

namespace {

ImVec4 level_color(spdlog::level::level_enum lvl) noexcept
{
    switch (lvl) {
        case spdlog::level::trace:
        case spdlog::level::debug:
            return ImVec4(0.50f, 0.50f, 0.50f, 1.0f);   // grey
        case spdlog::level::warn:
            return ImVec4(1.00f, 0.80f, 0.20f, 1.0f);   // yellow
        case spdlog::level::err:
        case spdlog::level::critical:
            return ImVec4(1.00f, 0.30f, 0.30f, 1.0f);   // red
        default:
            return ImVec4(0.90f, 0.90f, 0.90f, 1.0f);   // near-white for info
    }
}

const char* level_prefix(spdlog::level::level_enum lvl) noexcept
{
    switch (lvl) {
        case spdlog::level::trace:    return "[TRC]";
        case spdlog::level::debug:    return "[DBG]";
        case spdlog::level::info:     return "[INF]";
        case spdlog::level::warn:     return "[WRN]";
        case spdlog::level::err:      return "[ERR]";
        case spdlog::level::critical: return "[CRT]";
        default:                      return "[???]";
    }
}

}  // namespace

void render_log_pane(float width, float height)
{
    UILogSink* sink = ui_log_sink();

    // ── Per-frame state ───────────────────────────────────────────────────────
    static int                    s_last_gen = -1;
    static std::vector<UILogEntry> s_entries;

    const int cur_gen   = sink ? sink->generation() : 0;
    const bool new_data = (cur_gen != s_last_gen);

    if (sink && new_data) {
        s_entries  = sink->snapshot();
        s_last_gen = cur_gen;
    }

    // ── Header row ────────────────────────────────────────────────────────────
    constexpr float header_h = 22.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.14f, 1.0f));
    ImGui::BeginChild("##LogHeader", ImVec2(width, header_h), ImGuiChildFlags_None);

    ImGui::SetCursorPosY(3.0f);
    ImGui::SetCursorPosX(8.0f);
    ImGui::TextDisabled("LOG");

    const float btn_x = width - 52.0f;
    if (btn_x > 50.0f) {
        ImGui::SameLine(btn_x);
        if (ImGui::SmallButton("Clear") && sink) {
            sink->clear();
            s_entries.clear();
            s_last_gen = sink->generation();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ── Scrollable content ────────────────────────────────────────────────────
    const float scroll_h = height - header_h;
    if (scroll_h < 4.0f) return;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));
    ImGui::BeginChild("##LogScroll", ImVec2(width, scroll_h),
                       ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(s_entries.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            const auto& e = s_entries[static_cast<std::size_t>(i)];
            ImGui::PushStyleColor(ImGuiCol_Text, level_color(e.level));
            ImGui::Text("%s %s  %s", e.time_str.c_str(),
                        level_prefix(e.level), e.message.c_str());
            ImGui::PopStyleColor();
        }
    }
    clipper.End();

    // Auto-scroll to bottom when new entries arrive
    if (new_data)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace fastscope
