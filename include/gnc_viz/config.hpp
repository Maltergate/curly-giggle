#pragma once
/// @file config.hpp
/// @brief Persistent application configuration (color palette, layout, preferences).
/// @ingroup utilities

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace gnc_viz {

/// @brief 20-entry perceptually-distinct colorblind-friendly RGBA palette.
/// @details Colors are stored as packed 0xRRGGBBAA uint32 values.
///          Defaults to an IBM/Wong 8-color base extended to 20 with Tableau variations.
struct Palette {
    static constexpr int kSize = 20;

    // Default: IBM/Wong 8-color base extended to 20 with Tableau variations
    std::array<uint32_t, kSize> colors = {
        0x4477AAFF, 0xEE6677FF, 0x228833FF, 0xCCBB44FF,
        0x66CCEEFF, 0xAA3377FF, 0xBBBBBBFF, 0x000000FF,
        0xFF6600FF, 0x0066FFFF, 0x00CC88FF, 0xFF99CCFF,
        0x993300FF, 0x33AAFFFF, 0x88FF00FF, 0xFF0066FF,
        0x00AAFFFF, 0xFFAA00FF, 0x6600FFFF, 0x00FF99FF,
    };
};

// ── Config ────────────────────────────────────────────────────────────────────

/// @brief Persistent application configuration loaded from and saved to JSON.
/// @details Loaded from ~/.gnc_viz/config.json on startup; saved on shutdown.
///          AppState owns a Config instance; UI panels read/write its fields directly.
struct Config {
    // Layout
    float file_pane_width   = 280.0f;
    float signal_pane_width = 300.0f;

    // Appearance
    float font_size    = 14.0f;
    bool  dark_theme   = true;

    // Behaviour
    bool  restore_session = true;  // load last session on startup

    // Colors
    Palette palette;

    // ── Persistence ───────────────────────────────────────────────────────────

    /// Default path: ~/.gnc_viz/config.json
    static std::filesystem::path default_path();

    /// Load config from path.  Returns default Config if file doesn't exist.
    static Config load(const std::filesystem::path& path = default_path());

    /// Save config to path.  Creates parent directories if needed.
    /// Returns false and logs a warning on failure (non-fatal).
    bool save(const std::filesystem::path& path = default_path()) const;
};

} // namespace gnc_viz
