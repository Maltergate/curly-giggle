#include "fastscope/config.hpp"
#include "fastscope/log.hpp"

#include <nlohmann/json.hpp>
#include <fstream>

namespace fastscope {

// ── JSON serialisation helpers ────────────────────────────────────────────────

static void to_json(nlohmann::json& j, const Palette& p)
{
    j = nlohmann::json::array();
    for (auto c : p.colors)
        j.push_back(c);
}

static void from_json(const nlohmann::json& j, Palette& p)
{
    for (std::size_t i = 0; i < Palette::kSize && i < j.size(); ++i)
        p.colors[i] = j[i].get<uint32_t>();
}

static void to_json(nlohmann::json& j, const Config& c)
{
    j = {
        {"file_pane_width",   c.file_pane_width},
        {"signal_pane_width", c.signal_pane_width},
        {"font_size",         c.font_size},
        {"dark_theme",        c.dark_theme},
        {"restore_session",   c.restore_session},
        {"palette",           c.palette},
    };
}

static void from_json(const nlohmann::json& j, Config& c)
{
    if (j.contains("file_pane_width"))   j.at("file_pane_width").get_to(c.file_pane_width);
    if (j.contains("signal_pane_width")) j.at("signal_pane_width").get_to(c.signal_pane_width);
    if (j.contains("font_size"))         j.at("font_size").get_to(c.font_size);
    if (j.contains("dark_theme"))        j.at("dark_theme").get_to(c.dark_theme);
    if (j.contains("restore_session"))   j.at("restore_session").get_to(c.restore_session);
    if (j.contains("palette"))           j.at("palette").get_to(c.palette);
}

// ── Config methods ────────────────────────────────────────────────────────────

std::filesystem::path Config::default_path()
{
    const char* home = std::getenv("HOME");
    std::filesystem::path base = home ? std::filesystem::path(home) : std::filesystem::current_path();
    return base / ".fastscope" / "config.json";
}

Config Config::load(const std::filesystem::path& path)
{
    Config cfg;
    if (!std::filesystem::exists(path)) {
        FASTSCOPE_LOG_DEBUG("Config file not found at {}, using defaults", path.string());
        return cfg;
    }

    try {
        std::ifstream f(path);
        if (!f.is_open()) {
            FASTSCOPE_LOG_WARN("Cannot open config file: {}", path.string());
            return cfg;
        }
        nlohmann::json j;
        f >> j;
        from_json(j, cfg);
        FASTSCOPE_LOG_INFO("Config loaded from {}", path.string());
    } catch (const std::exception& e) {
        FASTSCOPE_LOG_WARN("Failed to parse config ({}): {} — using defaults", path.string(), e.what());
    }
    return cfg;
}

bool Config::save(const std::filesystem::path& path) const
{
    try {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream f(path);
        if (!f.is_open()) {
            FASTSCOPE_LOG_WARN("Cannot write config to: {}", path.string());
            return false;
        }
        nlohmann::json j;
        to_json(j, *this);
        f << j.dump(2) << "\n";
        FASTSCOPE_LOG_DEBUG("Config saved to {}", path.string());
        return true;
    } catch (const std::exception& e) {
        FASTSCOPE_LOG_WARN("Config save failed: {}", e.what());
        return false;
    }
}

} // namespace fastscope
