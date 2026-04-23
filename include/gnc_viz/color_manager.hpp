#pragma once

// ── ColorManager — assign distinct plot colors to signals ─────────────────────
//
// Manages a fixed palette of perceptually-distinct colors.
// Each signal key gets a unique color from the palette.
// When a signal is removed from the plot, its color is released for reuse.
//
// Colors are stored as packed RGBA uint32 (0xRRGGBBAA), compatible with
// ImGui's IM_COL32 format and Palette::colors.
//
// Usage:
//   ColorManager cm(config.palette);
//   uint32_t col = cm.assign("sim0//attitude/quat");
//   // ... later ...
//   cm.release("sim0//attitude/quat");

#include "gnc_viz/config.hpp"   // for Palette

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace gnc_viz {

class ColorManager {
public:
    explicit ColorManager(const Palette& palette = {});

    // ── Assignment ────────────────────────────────────────────────────────────

    /// Assign a color to signal_key.  If already assigned, returns same color.
    /// If the palette is exhausted, colors wrap around (modulo).
    [[nodiscard]] uint32_t assign(const std::string& signal_key);

    /// Release the color held by signal_key so it can be reused.
    /// No-op if signal_key has no assigned color.
    void release(const std::string& signal_key);

    /// Release all assigned colors.
    void clear();

    // ── Query ─────────────────────────────────────────────────────────────────

    /// Returns the assigned color, or std::nullopt if not assigned.
    [[nodiscard]] std::optional<uint32_t> get(const std::string& signal_key) const;

    /// Returns the assigned color, or a fallback gray if not assigned.
    [[nodiscard]] uint32_t get_or_default(const std::string& signal_key) const;

    /// Number of currently-assigned colors.
    [[nodiscard]] std::size_t assigned_count() const noexcept;

    /// Palette size.
    [[nodiscard]] std::size_t palette_size() const noexcept;

    // ── Palette management ────────────────────────────────────────────────────

    /// Replace the palette (releases all existing assignments).
    void set_palette(const Palette& palette);

    // ── Conversion helpers ────────────────────────────────────────────────────

    /// Convert packed RGBA uint32 to 4 normalized floats [0,1] (RGBA order).
    static void to_float4(uint32_t rgba, float out[4]) noexcept;

    /// Build a packed RGBA uint32 from normalized floats [0,1].
    static uint32_t from_float4(float r, float g, float b, float a = 1.0f) noexcept;

private:
    Palette                              m_palette;
    std::unordered_map<std::string, int> m_assigned;   // signal_key → palette index
    int                                  m_next_idx = 0;
};

} // namespace gnc_viz
