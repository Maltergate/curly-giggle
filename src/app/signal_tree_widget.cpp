// signal_tree_widget.cpp — Middle pane: hierarchical signal browser

#include "fastscope/signal_tree_widget.hpp"
#include "fastscope/app_state.hpp"
#include "fastscope/plotted_signal.hpp"
#include "fastscope/simulation_file.hpp"

#include "imgui.h"

#include <algorithm>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

namespace fastscope {

// ── Helpers ───────────────────────────────────────────────────────────────────

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

// ── Exclusion cache ───────────────────────────────────────────────────────────
//
// Rebuilt only when signal_exclusion_version changes — never on every frame.
// Each entry covers one loaded simulation (identified by sim_id).

struct SimExclusionCache {
    std::string       sim_id;
    uint32_t          version = UINT32_MAX;  // UINT32_MAX = never built
    std::vector<bool> excluded;              // indexed by sim.signals() position
};

static std::vector<SimExclusionCache> s_excl_cache;

/// Locate or create the cache entry for `sim`, and rebuild it if the
/// exclusion version has advanced since the last build.
static const std::vector<bool>&
refresh_exclusion_cache(const SimulationFile& sim, const AppState& state)
{
    const std::string& sid = sim.sim_id();

    // Find or create cache entry.
    SimExclusionCache* entry = nullptr;
    for (auto& c : s_excl_cache) {
        if (c.sim_id == sid) { entry = &c; break; }
    }
    if (!entry) {
        s_excl_cache.push_back({sid, UINT32_MAX, {}});
        entry = &s_excl_cache.back();
    }

    if (entry->version == state.signal_exclusion_version)
        return entry->excluded;  // cache is still valid

    // Rebuild: test every signal path against every compiled rule.
    const auto& signals = sim.signals();
    entry->excluded.assign(signals.size(), false);

    for (std::size_t i = 0; i < signals.size(); ++i) {
        const std::string& path = signals[i].h5_path;
        for (const auto& rule : state.signal_exclusions) {
            if (std::regex_search(path, rule.compiled)) {
                entry->excluded[i] = true;
                break;
            }
        }
    }

    entry->version = state.signal_exclusion_version;
    return entry->excluded;
}

/// Remove stale cache entries (sims that have been unloaded).
static void prune_exclusion_cache(const AppState& state)
{
    s_excl_cache.erase(
        std::remove_if(s_excl_cache.begin(), s_excl_cache.end(),
                       [&](const SimExclusionCache& c) {
                           for (const auto& s : state.simulations)
                               if (s->sim_id() == c.sim_id) return false;
                           return true;  // sim no longer loaded
                       }),
        s_excl_cache.end());
}

// ── Search / input state (file-scoped, persists across frames) ────────────────
static char s_search_buf[256] = {};
static char s_excl_input[256] = {};
static bool s_excl_error      = false;  // true when last typed pattern was invalid

// ── Widget ────────────────────────────────────────────────────────────────────

void render_signal_tree(AppState& state)
{
    prune_exclusion_cache(state);

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

    // Precompute lowercase filter once (not per-signal).
    std::string lower_filter(s_search_buf);
    if (!lower_filter.empty()) {
        std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }

    ImGui::Separator();

    // ── Height budget: reserve space for the exclusion panel at the bottom ────
    const float row_h      = ImGui::GetFrameHeightWithSpacing();
    const int   n_rules    = static_cast<int>(state.signal_exclusions.size());
    // input row + optional error row + separator + label + rule rows (capped at 5 visible)
    const int   visible_rules = std::min(n_rules, 5);
    const float excl_h     = row_h * (2.0f + static_cast<float>(visible_rules))
                             + ImGui::GetStyle().ItemSpacing.y * 3.0f
                             + 18.0f;  // separator + "Exclusions" text

    const float list_h = ImGui::GetContentRegionAvail().y - excl_h;

    // ── Signal list (scrollable child) ────────────────────────────────────────
    ImGui::BeginChild("##signal_list",
                      ImVec2(0.0f, list_h > 40.0f ? list_h : 40.0f),
                      ImGuiChildFlags_None);

    for (auto& sim_ptr : state.simulations) {
        auto& sim = *sim_ptr;

        const bool node_open = ImGui::TreeNodeEx(
            sim.display_name().c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (!node_open) continue;

        const auto& signals  = sim.signals();
        const auto& excluded = refresh_exclusion_cache(sim, state);

        // Build a per-simulation set of plotted h5_paths for O(1) lookup.
        std::unordered_set<std::string> plotted_paths;
        plotted_paths.reserve(state.plotted_signals.size());
        for (const auto& ps : state.plotted_signals) {
            if (ps.sim_id == sim.sim_id())
                plotted_paths.insert(ps.meta.h5_path);
        }

        // Pre-filter: exclusions + search filter → index array for the clipper.
        std::vector<int> visible;
        visible.reserve(signals.size());
        for (int i = 0; i < static_cast<int>(signals.size()); ++i) {
            if (excluded[i]) continue;
            if (!lower_filter.empty()) {
                const auto& path = signals[i].h5_path;
                auto it = std::search(path.begin(), path.end(),
                                      lower_filter.begin(), lower_filter.end(),
                                      [](unsigned char a, unsigned char b) {
                                          return std::tolower(a) == b;
                                      });
                if (it == path.end()) continue;
            }
            visible.push_back(i);
        }

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(visible.size()));
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const SignalMetadata& meta    = signals[visible[row]];
                const bool            in_plot = plotted_paths.count(meta.h5_path) > 0;

                ImGui::PushID(meta.h5_path.c_str());

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
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove from plot");
                } else {
                    if (ImGui::SmallButton("+")) {
                        PlottedSignal ps;
                        ps.sim_id     = sim.sim_id();
                        ps.meta       = meta;
                        ps.alias      = meta.h5_path;
                        ps.color_rgba = state.colors.assign(ps.plot_key());
                        state.plotted_signals.push_back(std::move(ps));
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add to plot");
                }
                ImGui::SameLine();

                const char* icon = meta.is_vector() ? "[v]" : "[s]";
                ImGui::TextDisabled("%s", icon);
                ImGui::SameLine();

                ImGui::TextUnformatted(meta.h5_path.c_str());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", meta.h5_path.c_str());

                const std::string shp = shape_str(meta);
                if (!shp.empty()) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", shp.c_str());
                }
                if (!meta.units.empty()) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("[%s]", meta.units.c_str());
                }

                ImGui::PopID();
            }
        }
        clipper.End();

        ImGui::TreePop();
    }

    ImGui::EndChild();  // ##signal_list

    // ── Exclusion rules panel (pinned at bottom of pane) ──────────────────────
    ImGui::Separator();
    ImGui::TextDisabled("Exclusions");

    // Input row: text field + [+] button
    const float btn_w   = ImGui::CalcTextSize("[+]").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float input_w = ImGui::GetContentRegionAvail().x - btn_w
                          - ImGui::GetStyle().ItemSpacing.x;

    ImGui::SetNextItemWidth(input_w);
    const bool enter_pressed = ImGui::InputTextWithHint(
        "##excl_input", "regex, e.g. ^/events/",
        s_excl_input, sizeof(s_excl_input),
        ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::IsItemEdited()) s_excl_error = false;  // clear error while typing

    ImGui::SameLine();
    const bool add_clicked = ImGui::Button("[+]##excl_add");

    if (enter_pressed || add_clicked) {
        const std::string pat(s_excl_input);
        if (!pat.empty()) {
            try {
                std::regex compiled(pat, std::regex::ECMAScript | std::regex::optimize);
                state.signal_exclusions.push_back({pat, std::move(compiled)});
                ++state.signal_exclusion_version;
                s_excl_input[0] = '\0';
                s_excl_error    = false;
            } catch (const std::regex_error&) {
                s_excl_error = true;
            }
        }
    }

    if (s_excl_error)
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Invalid regex pattern");

    // Rule list (scrollable when more than 5 rules)
    const float rules_child_h = row_h * static_cast<float>(visible_rules)
                                + ImGui::GetStyle().WindowPadding.y * 2.0f;
    if (n_rules > 0) {
        ImGui::BeginChild("##excl_rules",
                          ImVec2(0.0f, rules_child_h),
                          ImGuiChildFlags_FrameStyle);
        for (int i = 0; i < static_cast<int>(state.signal_exclusions.size()); ) {
            ImGui::PushID(i);
            if (ImGui::SmallButton("[x]")) {
                state.signal_exclusions.erase(state.signal_exclusions.begin() + i);
                ++state.signal_exclusion_version;
                ImGui::PopID();
                continue;
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(state.signal_exclusions[i].pattern.c_str());
            ImGui::PopID();
            ++i;
        }
        ImGui::EndChild();
    }
}

} // namespace fastscope
