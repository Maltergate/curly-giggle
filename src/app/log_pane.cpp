// log_pane.cpp — Floating log window showing recent application log entries.

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
            return ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
        case spdlog::level::warn:
            return ImVec4(1.00f, 0.80f, 0.20f, 1.0f);
        case spdlog::level::err:
        case spdlog::level::critical:
            return ImVec4(1.00f, 0.30f, 0.30f, 1.0f);
        default:
            return ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
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

void render_log_window(bool* open)
{
    UILogSink* sink = ui_log_sink();

    static int                     s_last_gen = -1;
    static std::vector<UILogEntry> s_entries;

    // Set a reasonable default size on first show
    ImGui::SetNextWindowSize(ImVec2(700.0f, 250.0f), ImGuiCond_FirstUseEver);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, 0.95f));
    const bool visible = ImGui::Begin("Log##LogWindow", open);
    ImGui::PopStyleColor();

    if (!visible) {
        ImGui::End();
        return;
    }

    // Refresh entries when the sink has new data
    const int cur_gen  = sink ? sink->generation() : 0;
    const bool new_data = (cur_gen != s_last_gen);
    if (sink && new_data) {
        s_entries  = sink->snapshot();
        s_last_gen = cur_gen;
    }

    // Clear button aligned to the right
    const float btn_w = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - btn_w);
    if (ImGui::SmallButton("Clear") && sink) {
        sink->clear();
        s_entries.clear();
        s_last_gen = sink->generation();
    }

    ImGui::Separator();

    // Scrollable log body
    ImGui::BeginChild("##LogBody", ImVec2(0.0f, 0.0f),
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

    if (new_data)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

} // namespace fastscope
