#include "fastscope/axis_manager.hpp"

#include <algorithm>

namespace fastscope {

AxisManager::AxisManager()
{
    for (int i = 0; i < max_axes; ++i)
        m_configs[i].id = i;
}

void AxisManager::assign(const std::string& plot_key, int axis_id)
{
    axis_id = std::clamp(axis_id, 0, max_axes - 1);
    m_assignments[plot_key] = axis_id;
}

void AxisManager::release(const std::string& plot_key)
{
    m_assignments.erase(plot_key);
}

int AxisManager::get_axis_for(const std::string& plot_key) const
{
    auto it = m_assignments.find(plot_key);
    return (it != m_assignments.end()) ? it->second : 0;
}

std::vector<AxisConfig> AxisManager::active_axes() const
{
    std::vector<AxisConfig> result;
    result.push_back(m_configs[0]); // axis 0 always included
    for (int i = 1; i < max_axes; ++i) {
        if (has_signals_on(i))
            result.push_back(m_configs[i]);
    }
    return result;
}

AxisConfig& AxisManager::axis_config(int axis_id)
{
    axis_id = std::clamp(axis_id, 0, max_axes - 1);
    return m_configs[axis_id];
}

bool AxisManager::has_signals_on(int axis_id) const
{
    for (const auto& [key, id] : m_assignments) {
        if (id == axis_id) return true;
    }
    return false;
}

int AxisManager::active_axis_count() const
{
    // Count distinct axis ids that have at least one signal
    bool seen[max_axes] = {};
    for (const auto& [key, id] : m_assignments)
        if (id >= 0 && id < max_axes) seen[id] = true;
    int count = 0;
    for (bool b : seen) count += b ? 1 : 0;
    return count;
}

void AxisManager::clear()
{
    m_assignments.clear();
    for (int i = 0; i < max_axes; ++i) {
        m_configs[i] = AxisConfig{};
        m_configs[i].id = i;
    }
}

void AxisManager::clear_assignments()
{
    m_assignments.clear();
}

} // namespace fastscope
