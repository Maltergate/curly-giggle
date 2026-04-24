// signal_tree_widget.cpp — Middle pane: hierarchical signal browser

#include "fastscope/signal_tree_widget.hpp"
#include "fastscope/app_state.hpp"
#include "fastscope/plotted_signal.hpp"
#include "fastscope/simulation_file.hpp"

#include "imgui.h"

#include <algorithm>
#include <string>

namespace fastscope {

// ── Helpers ───────────────────────────────────────────────────────────────────

static bool is_plotted(const AppState& state,
                       const std::string& sim_id,
                       const std::string& h5_path)
{
    return std::any_of(state.plotted_signals.begin(), state.plotted_signals.end(),
                       [&](const PlottedSignal& p) {
                           return p.sim_id == sim_id && p.meta.h5_path == h5_path;
                       });
}

/// Format signal shape as "(N)" or "(N×K)" etc.
static std::string shape_str(const SignalMetadata& meta)
{
    if (meta.shape.empty()) return "";
    std::string s = "(";
    for (std::size_t i = 0; i < meta.shape.size(); ++i) {
        if (i) s += "\xC3\x97";  // UTF-8 "×"
        s += std::to_string(meta.shape[i]);
    }
    return s + ")";
}

// ── Search state (file-scoped, persists across frames) ────────────────────────
static char s_search_buf[256] = {};

// ── Widget ────────────────────────────────────────────────────────────────────

void render_signal_tree(AppState& state)
{
    // ── Header ────────────────────────────────────────────────────────────────
    ImGui::TextUnformatted("Signals");
    ImGui::Separator();

    if (state.simulations.empty()) {
        ImGui::Spacing();
        ImGui::TextDisabled("Load a simulation to browse");
        ImGui::TextDisabled("its signals.");
        return;
    }

    // ── Search bar ────────────────────────────────────────────────────────────
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##search", "Filter signals…", s_search_buf, sizeof(s_search_buf));
    const std::string filter(s_search_buf);

    // Precompute lowercase filter once (not per-signal).
    std::string lower_filter;
    if (!filter.empty()) {
        lower_filter = filter;
        std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }

    ImGui::Separator();

    // ── Per-simulation trees ──────────────────────────────────────────────────
    for (auto& sim_ptr : state.simulations) {
        auto& sim = *sim_ptr;

        const bool node_open = ImGui::TreeNodeEx(
            sim.display_name().c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (!node_open) continue;

        for (const auto& meta : sim.signals()) {
            // Apply filter
            if (!lower_filter.empty()) {
                std::string lower_name = meta.h5_path;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                               [](unsigned char c){ return std::tolower(c); });
                if (lower_name.find(lower_filter) == std::string::npos) continue;
            }

            const bool in_plot = is_plotted(state, sim.sim_id(), meta.h5_path);

            ImGui::PushID(meta.h5_path.c_str());

            // [+]/[-] toggle on the LEFT — never overlaps with signal text
            if (in_plot) {
                if (ImGui::SmallButton("-")) {
                    state.plotted_signals.erase(
                        std::remove_if(state.plotted_signals.begin(),
                                       state.plotted_signals.end(),
                                       [&](const PlottedSignal& p) {
                                           return p.sim_id == sim.sim_id() &&
                                                  p.meta.h5_path == meta.h5_path;
                                       }),
                        state.plotted_signals.end());
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Remove from plot");
            } else {
                if (ImGui::SmallButton("+")) {
                    PlottedSignal ps;
                    ps.sim_id     = sim.sim_id();
                    ps.meta       = meta;
                    ps.alias      = meta.h5_path;
                    ps.color_rgba = state.colors.assign(ps.plot_key());
                    state.plotted_signals.push_back(std::move(ps));
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Add to plot");
            }
            ImGui::SameLine();

            // Type icon: [v] for vector, [s] for scalar
            const char* icon = meta.is_vector() ? "[v]" : "[s]";
            ImGui::TextDisabled("%s", icon);
            ImGui::SameLine();

            // Full HDF5 path — users need the full path to distinguish
            // signals with the same leaf name from different sources.
            ImGui::TextUnformatted(meta.h5_path.c_str());
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", meta.h5_path.c_str());

            // Shape annotation
            const std::string shp = shape_str(meta);
            if (!shp.empty()) {
                ImGui::SameLine();
                ImGui::TextDisabled("%s", shp.c_str());
            }

            // Units annotation
            if (!meta.units.empty()) {
                ImGui::SameLine();
                ImGui::TextDisabled("[%s]", meta.units.c_str());
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}

} // namespace fastscope
