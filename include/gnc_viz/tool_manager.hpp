#pragma once
/// @file tool_manager.hpp
/// @brief Owns the ToolRegistry and the currently active IVisualizationTool.
/// @defgroup tools Visualization Tools
/// @brief Interactive overlay tools: Annotation and Ruler.

#include "gnc_viz/registry.hpp"
#include "gnc_viz/interfaces.hpp"
#include <memory>
#include <string>
#include <string_view>

namespace gnc_viz {

struct AppState;

/// @brief Owns the ToolRegistry and the currently active IVisualizationTool.
/// @details One tool active at a time (or none = nullptr). Handles activate/deactivate
///          toggling and dispatches per-frame input and overlay calls.
class ToolManager {
public:
    /// @brief Construct and register all built-in tools (AnnotationTool, RulerTool).
    ToolManager();   // registers AnnotationTool, RulerTool

    /// Activate tool by id. If already active, deactivates it (toggle).
    /// Pass "" to deactivate all tools.
    void activate(std::string_view tool_id);

    /// Deactivate the current tool.
    void deactivate();

    /// Returns id of current tool, or "" if none.
    [[nodiscard]] std::string_view active_tool_id() const noexcept;

    /// Returns true if the named tool is currently active.
    [[nodiscard]] bool is_active(std::string_view tool_id) const noexcept;

    /// Returns all registered tool ids (for toolbar rendering).
    [[nodiscard]] const std::vector<std::string>& available_tools() const noexcept;

    /// Call each frame from timeseries_plot.cpp, inside BeginPlot block.
    /// Calls handle_input() and render_overlay() on the active tool.
    void tick(AppState& state);

    /// Direct access to active tool (may be nullptr).
    IVisualizationTool* active_tool() noexcept;

private:
    ToolRegistry                        m_registry;
    std::string                         m_active_id;
    std::unique_ptr<IVisualizationTool> m_active;
};

} // namespace gnc_viz
