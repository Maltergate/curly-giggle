// simulation_list_ui.cpp — Left pane: load and manage simulation files

#include "fastscope/simulation_list_ui.hpp"
#include "fastscope/app_state.hpp"
#include "fastscope/file_open_dialog.hpp"
#include "fastscope/simulation_file.hpp"
#include "fastscope/plotted_signal.hpp"
#include "fastscope/log.hpp"

#include "imgui.h"

#include <algorithm>
#include <string>

namespace fastscope {

// Counter for unique sim_ids — persists across calls (file-scoped).
static int s_sim_counter = 0;

// Rename editing state
static std::string s_editing_id;
static char        s_rename_buf[256] = {};
static bool        s_focus_rename    = false;

void render_simulation_list(AppState& state)
{
    // ── Header row: title + [+] open button ───────────────────────────────────
    ImGui::TextUnformatted("Simulations");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 22.0f);
    if (ImGui::Button("+##opensim", ImVec2(22, 0))) {
        auto result = show_open_dialog("Open Simulation File", /*multiple=*/true,
                                       {"h5", "hdf5"});
        if (result.confirmed) {
            for (const auto& p : result.paths) {
                const std::string id = "sim" + std::to_string(s_sim_counter++);
                auto sim = std::make_unique<SimulationFile>(id);
                if (auto res = sim->open(p); res)
                    state.simulations.push_back(std::move(sim));
                else
                    FASTSCOPE_LOG_ERROR("Failed to open '{}': {}", p.string(), res.error().message);
            }
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Open HDF5 simulation file(s)");

    ImGui::Separator();

    // ── List loaded simulations ───────────────────────────────────────────────
    if (state.simulations.empty()) {
        ImGui::Spacing();
        ImGui::TextDisabled("No simulations loaded.");
        ImGui::TextDisabled("Click [+] or drop .h5 files.");
        return;
    }

    for (int i = 0; i < static_cast<int>(state.simulations.size()); ) {
        auto& sim   = *state.simulations[i];
        bool  unload = false;

        ImGui::PushID(sim.sim_id().c_str());

        // ── Display name (double-click to rename) ─────────────────────────────
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "●");
        ImGui::SameLine();

        if (s_editing_id == sim.sim_id()) {
            if (s_focus_rename) {
                ImGui::SetKeyboardFocusHere();
                s_focus_rename = false;
            }
            ImGui::SetNextItemWidth(-1.0f);
            const bool enter = ImGui::InputText(
                "##rename", s_rename_buf, sizeof(s_rename_buf),
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll);
            // Commit on Enter or focus loss
            if (enter || (!ImGui::IsItemActive() && ImGui::IsItemDeactivated())) {
                if (s_rename_buf[0] != '\0')
                    sim.set_display_name(s_rename_buf);
                s_editing_id.clear();
            }
        } else {
            ImGui::TextUnformatted(sim.display_name().c_str());
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                s_editing_id   = sim.sim_id();
                s_focus_rename = true;
                std::strncpy(s_rename_buf, sim.display_name().c_str(),
                             sizeof(s_rename_buf) - 1);
                s_rename_buf[sizeof(s_rename_buf) - 1] = '\0';
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Double-click to rename");
        }

        // Sim ID + signal count hint
        ImGui::Indent(12.0f);
        ImGui::TextDisabled("%s  •  %zu datasets",
                            sim.sim_id().c_str(), sim.signals().size());

        // ── Time axis selector ────────────────────────────────────────────────
        ImGui::TextUnformatted("Time axis:");
        ImGui::SameLine();
        const auto& current = sim.time_axis();
        const char* preview = current.empty() ? "[sample index]" : current.c_str();

        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##ta", preview)) {
            // Option: use sample index
            if (ImGui::Selectable("[sample index]", current.empty())) {
                sim.set_time_axis("");
                // Invalidate buffers so they reload with the new time axis
                for (auto& ps : state.plotted_signals)
                    if (ps.sim_id == sim.sim_id())
                        ps.buffer.reset();
                sim.evict_cache();
            }
            // Heuristic suggestions
            for (const auto& hint : sim.suggested_time_axes()) {
                const bool sel = (hint == current);
                if (ImGui::Selectable(hint.c_str(), sel)) {
                    sim.set_time_axis(hint);
                    for (auto& ps : state.plotted_signals)
                        if (ps.sim_id == sim.sim_id())
                            ps.buffer.reset();
                    sim.evict_cache();
                }
            }
            ImGui::EndCombo();
        }

        // ── Unload button ─────────────────────────────────────────────────────
        if (ImGui::Button("Unload")) unload = true;
        ImGui::Unindent(12.0f);

        ImGui::PopID();
        ImGui::Separator();

        if (unload) {
            // Remove all plotted signals from this sim and release their colors
            auto& ps = state.plotted_signals;
            ps.erase(
                std::remove_if(ps.begin(), ps.end(), [&](const PlottedSignal& p) {
                    if (p.sim_id != sim.sim_id()) return false;
                    state.colors.release(p.plot_key());
                    return true;
                }),
                ps.end());
            state.simulations.erase(state.simulations.begin() + i);
        } else {
            ++i;
        }
    }
}

} // namespace fastscope
