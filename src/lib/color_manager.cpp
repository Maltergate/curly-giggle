#include "gnc_viz/color_manager.hpp"
#include <cassert>

namespace gnc_viz {

ColorManager::ColorManager(const Palette& palette)
    : m_palette(palette)
{}

uint32_t ColorManager::assign(const std::string& signal_key)
{
    auto it = m_assigned.find(signal_key);
    if (it != m_assigned.end())
        return m_palette.colors[it->second % Palette::kSize];

    int idx = m_next_idx % Palette::kSize;
    m_assigned[signal_key] = idx;
    ++m_next_idx;
    return m_palette.colors[idx];
}

void ColorManager::release(const std::string& signal_key)
{
    m_assigned.erase(signal_key);
}

void ColorManager::clear()
{
    m_assigned.clear();
    m_next_idx = 0;
}

std::optional<uint32_t> ColorManager::get(const std::string& signal_key) const
{
    auto it = m_assigned.find(signal_key);
    if (it == m_assigned.end()) return std::nullopt;
    return m_palette.colors[it->second % Palette::kSize];
}

uint32_t ColorManager::get_or_default(const std::string& signal_key) const
{
    auto c = get(signal_key);
    return c.value_or(0xAAAAAAAA);   // mid-gray fallback
}

std::size_t ColorManager::assigned_count() const noexcept
{
    return m_assigned.size();
}

std::size_t ColorManager::palette_size() const noexcept
{
    return Palette::kSize;
}

void ColorManager::set_palette(const Palette& palette)
{
    m_palette = palette;
    m_assigned.clear();
    m_next_idx = 0;
}

void ColorManager::to_float4(uint32_t rgba, float out[4]) noexcept
{
    out[0] = static_cast<float>((rgba >> 24) & 0xFF) / 255.0f;  // R
    out[1] = static_cast<float>((rgba >> 16) & 0xFF) / 255.0f;  // G
    out[2] = static_cast<float>((rgba >>  8) & 0xFF) / 255.0f;  // B
    out[3] = static_cast<float>((rgba >>  0) & 0xFF) / 255.0f;  // A
}

uint32_t ColorManager::from_float4(float r, float g, float b, float a) noexcept
{
    auto to_byte = [](float v) -> uint32_t {
        return static_cast<uint32_t>(v * 255.0f + 0.5f) & 0xFF;
    };
    return (to_byte(r) << 24) | (to_byte(g) << 16) | (to_byte(b) << 8) | to_byte(a);
}

} // namespace gnc_viz
