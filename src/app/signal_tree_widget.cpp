// signal_tree_widget.cpp — Middle pane: hierarchical signal browser

#include "gnc_viz/signal_tree_widget.hpp"
#include "gnc_viz/app_state.hpp"
#include "gnc_viz/plotted_signal.hpp"
#include "gnc_viz/simulation_file.hpp"

#include "imgui.h"

#include <algorithm>
#include <string>

namespace gnc_viz {

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
    ImGui::Separator();

    // ── Per-simulation trees ──────────────────────────────────────────────────
    for (auto& sim_ptr : state.simulations) {
        auto& sim = *sim_ptr;

        const bool node_open = ImGui::TreeNodeEx(
            sim.path().filename().string().c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (!node_open) continue;

        for (const auto& meta : sim.signals()) {
            // Apply filter
            if (!filter.empty()) {
                const std::string lower_name = [&]{
                    std::string s = meta.h5_path;
                    std::transform(s.begin(), s.end(), s.begin(),
                                   [](unsigned char c){ return std::tolower(c); });
                    return s;
                }();
                const std::string lower_filter = [&]{
                    std::string s = filter;
                    std::transform(s.begin(), s.end(), s.begin(),
                                   [](unsigned char c){ return std::tolower(c); });
                    return s;
                }();
                if (lower_name.find(lower_filter) == std::string::npos) continue;
            }

            const bool in_plot = is_plotted(state, sim.sim_id(), meta.h5_path);

            ImGui::PushID(meta.h5_path.c_str());

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

            // [+] / checkmark at right edge
            const float btn_w = 22.0f;
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - btn_w +
                            ImGui::GetCursorPosX());
            if (in_plot) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "✓");
            } else {
                if (ImGui::SmallButton("+##add")) {
                    PlottedSignal ps;
                    ps.sim_id    = sim.sim_id();
                    ps.meta      = meta;
                    ps.alias     = meta.h5_path;
                    ps.color_rgba = state.colors.assign(ps.plot_key());
                    state.plotted_signals.push_back(std::move(ps));
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Add to plot");
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}

} // namespace gnc_viz
