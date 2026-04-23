// tool_manager.cpp — ToolManager implementation

#include "gnc_viz/tool_manager.hpp"
#include "gnc_viz/annotation_tool.hpp"
#include "gnc_viz/ruler_tool.hpp"
#include "gnc_viz/app_state.hpp"

namespace gnc_viz {

ToolManager::ToolManager()
{
    m_registry.register_type<AnnotationTool>("annotation");
    m_registry.register_type<RulerTool>("ruler");
}

void ToolManager::activate(std::string_view tool_id)
{
    const std::string id{tool_id};

    // Empty string always deactivates
    if (id.empty()) {
        deactivate();
        return;
    }

    // Same id as active → toggle off
    if (id == m_active_id) {
        deactivate();
        return;
    }

    // Deactivate current (if any)
    if (m_active) {
        m_active->on_deactivate();
        m_active.reset();
        m_active_id.clear();
    }

    // Create new tool from registry; silently ignore unknown ids
    auto tool = m_registry.create(id);
    if (!tool) return;

    m_active    = std::move(tool);
    m_active_id = id;
    m_active->on_activate();
}

void ToolManager::deactivate()
{
    if (m_active) {
        m_active->on_deactivate();
        m_active.reset();
    }
    m_active_id.clear();
}

std::string_view ToolManager::active_tool_id() const noexcept
{
    return m_active_id;
}

bool ToolManager::is_active(std::string_view tool_id) const noexcept
{
    return !m_active_id.empty() && m_active_id == tool_id;
}

const std::vector<std::string>& ToolManager::available_tools() const noexcept
{
    return m_registry.keys();
}

void ToolManager::tick(AppState& state)
{
    if (m_active) {
        m_active->handle_input(state);
        m_active->render_overlay(state);
    }
}

IVisualizationTool* ToolManager::active_tool() noexcept
{
    return m_active.get();
}

} // namespace gnc_viz
