#pragma once
/// @file collapsible_pane.hpp
/// @brief Collapsible side-pane ImGui widget.
/// @ingroup ui_widgets

#include "imgui.h"
#include <string>
#include <string_view>

namespace gnc_viz {

/// @brief Collapsible side-pane ImGui widget.
/// @details Renders a child window that can be collapsed to a 22-px icon strip.
///          Normal state shows a full-width pane with a collapse button in the header.
///          The caller must call end() if and only if begin() returns true.
class CollapsiblePane {
public:
    static constexpr float kCollapsedWidth = 22.0f;
    static constexpr float kHeaderHeight   = 24.0f;

    /// @param label       Text shown in the header when expanded.
    /// @param id          Unique ImGui ID for the child window (e.g. "##files").
    /// @param visible     Pointer to the visibility bool in AppState.panes.
    CollapsiblePane(std::string_view label, std::string_view id, bool* visible)
        : m_label(label), m_id(id), m_visible(visible) {}

    /// Begin the pane.
    /// @param width    Current expanded width (from AppState.panes.*_width).
    /// @param avail_h  Available vertical height for the pane child window.
    /// @returns true  → caller must call end() and may render content.
    ///          false → pane is collapsed; do NOT call end().
    bool begin(float width, float avail_h);

    /// End the pane.  MUST be called if and only if begin() returned true.
    void end();

    /// Returns the effective rendered width (collapsed or expanded).
    [[nodiscard]] float rendered_width() const noexcept { return m_last_width; }

private:
    std::string m_label;
    std::string m_id;
    bool*       m_visible    = nullptr;
    float       m_last_width = 0.0f;
};

} // namespace gnc_viz
