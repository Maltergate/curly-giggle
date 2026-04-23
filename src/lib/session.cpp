#include "gnc_viz/session.hpp"
#include "gnc_viz/app_state.hpp"
#include "gnc_viz/simulation_file.hpp"
#include "gnc_viz/axis_manager.hpp"
#include "gnc_viz/log.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <cstdio>

namespace gnc_viz {

std::filesystem::path default_session_path()
{
    const char* home = std::getenv("HOME");
    std::filesystem::path base = home
        ? std::filesystem::path(home)
        : std::filesystem::current_path();
    return base / ".gnc_viz" / "session.json";
}

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string color_to_hex(uint32_t rgba)
{
    char buf[12];
    std::snprintf(buf, sizeof(buf), "0x%08X", rgba);
    return buf;
}

static uint32_t hex_to_color(const std::string& s)
{
    try {
        return static_cast<uint32_t>(std::stoul(s, nullptr, 0));
    } catch (...) {
        return 0xFFFFFFFFu;
    }
}

// ── save_session ──────────────────────────────────────────────────────────────

bool save_session(const AppState& state, const std::filesystem::path& path)
{
    using json = nlohmann::json;
    try {
        json j;
        j["version"] = 1;

        // Simulations
        json sims = json::array();
        for (const auto& sim : state.simulations)
            sims.push_back({{"path", sim->path().string()}, {"sim_id", sim->sim_id()}});
        j["simulations"] = std::move(sims);

        // PlottedSignals (buffer excluded — reloaded on open)
        json signals = json::array();
        for (const auto& ps : state.plotted_signals) {
            signals.push_back({
                {"sim_id",          ps.sim_id},
                {"h5_path",         ps.meta.h5_path},
                {"alias",           ps.alias},
                {"color_rgba",      color_to_hex(ps.color_rgba)},
                {"y_axis",          ps.y_axis},
                {"visible",         ps.visible},
                {"component_index", ps.component_index}
            });
        }
        j["plotted_signals"] = std::move(signals);

        // Y-axes
        json y_axes = json::array();
        for (const auto& ax : state.axis_manager.active_axes()) {
            y_axes.push_back({
                {"id",         ax.id},
                {"label",      ax.label},
                {"min",        ax.range_min},
                {"max",        ax.range_max},
                {"auto_range", ax.auto_range}
            });
        }
        j["y_axes"] = std::move(y_axes);

        j["active_plot_type"] = "timeseries";

        // TODO: serialize annotations (needs AnnotationTool access when not active)
        j["annotations"] = json::array();

        // Panes
        j["panes"] = {
            {"file_pane_visible",   state.panes.file_pane_visible},
            {"signal_pane_visible", state.panes.signal_pane_visible},
            {"file_pane_width",     state.panes.file_pane_width},
            {"signal_pane_width",   state.panes.signal_pane_width}
        };

        std::filesystem::create_directories(path.parent_path());

        std::ofstream ofs(path);
        if (!ofs) {
            GNC_LOG_WARN("save_session: cannot open '{}' for writing", path.string());
            return false;
        }
        ofs << j.dump(2) << '\n';
        GNC_LOG_INFO("save_session: saved to '{}'", path.string());
        return true;

    } catch (const std::exception& e) {
        GNC_LOG_WARN("save_session: exception: {}", e.what());
        return false;
    }
}

// ── load_session ──────────────────────────────────────────────────────────────

bool load_session(AppState& state, const std::filesystem::path& path)
{
    using json = nlohmann::json;

    if (!std::filesystem::exists(path)) {
        GNC_LOG_DEBUG("load_session: file not found '{}'", path.string());
        return false;
    }

    std::ifstream ifs(path);
    if (!ifs) {
        GNC_LOG_WARN("load_session: cannot open '{}' for reading", path.string());
        return false;
    }

    json j;
    try {
        j = json::parse(ifs);
    } catch (const json::exception& e) {
        GNC_LOG_WARN("load_session: JSON parse error in '{}': {}", path.string(), e.what());
        return false;
    }

    if (j.value("version", 0) != 1) {
        GNC_LOG_WARN("load_session: unsupported session version in '{}'", path.string());
        return false;
    }

    // Simulations
    for (const auto& entry : j.value("simulations", json::array())) {
        std::string sim_path = entry.value("path", "");
        std::string sim_id   = entry.value("sim_id", "");
        if (sim_path.empty() || sim_id.empty()) continue;

        auto sim = std::make_unique<SimulationFile>(sim_id);
        if (auto res = sim->open(sim_path); res) {
            state.simulations.push_back(std::move(sim));
        } else {
            GNC_LOG_WARN("load_session: could not open '{}': {}", sim_path, res.error().message);
        }
    }

    // PlottedSignals — requires the simulations to be loaded first
    for (const auto& entry : j.value("plotted_signals", json::array())) {
        const std::string sim_id  = entry.value("sim_id",  "");
        const std::string h5_path = entry.value("h5_path", "");
        if (sim_id.empty() || h5_path.empty()) continue;

        SimulationFile* sim = nullptr;
        for (const auto& s : state.simulations) {
            if (s->sim_id() == sim_id) { sim = s.get(); break; }
        }
        if (!sim) {
            GNC_LOG_WARN("load_session: sim_id '{}' not found for signal '{}'", sim_id, h5_path);
            continue;
        }

        const SignalMetadata* meta_ptr = nullptr;
        for (const auto& meta : sim->signals()) {
            if (meta.h5_path == h5_path) { meta_ptr = &meta; break; }
        }
        if (!meta_ptr) {
            GNC_LOG_WARN("load_session: signal '{}' not found in sim '{}'", h5_path, sim_id);
            continue;
        }

        PlottedSignal ps;
        ps.sim_id          = sim_id;
        ps.meta            = *meta_ptr;
        ps.alias           = entry.value("alias", "");
        ps.y_axis          = entry.value("y_axis", 0);
        ps.visible         = entry.value("visible", true);
        ps.component_index = entry.value("component_index", -1);
        ps.color_rgba      = hex_to_color(entry.value("color_rgba", "0xFFFFFFFF"));

        state.axis_manager.assign(ps.plot_key(), ps.y_axis);
        state.plotted_signals.push_back(std::move(ps));
    }

    // Y-axis configs
    for (const auto& entry : j.value("y_axes", json::array())) {
        int id = entry.value("id", 0);
        auto& cfg      = state.axis_manager.axis_config(id);
        cfg.label      = entry.value("label", "");
        cfg.auto_range = entry.value("auto_range", true);
        cfg.range_min  = entry.value("min", 0.0);
        cfg.range_max  = entry.value("max", 1.0);
    }

    // Panes
    if (j.contains("panes")) {
        const auto& panes             = j["panes"];
        state.panes.file_pane_visible   = panes.value("file_pane_visible",   true);
        state.panes.signal_pane_visible = panes.value("signal_pane_visible", true);
        state.panes.file_pane_width     = panes.value("file_pane_width",     280.0f);
        state.panes.signal_pane_width   = panes.value("signal_pane_width",   300.0f);
    }

    GNC_LOG_INFO("load_session: restored from '{}'", path.string());
    return true;
}

} // namespace gnc_viz
