#pragma once

// ── CollapsiblePane — generic collapsible side-pane widget ───────────────────
//
// Renders a child window with:
//   - Normal state:  full width with a collapse button (◀ / ▶) in the header
//   - Collapsed state: 22px-wide icon strip with an expand button (▶ / ◀)
//
// Usage (inside the host window, before calling BeginChild for your content):
//
//   static CollapsiblePane left_pane("Files", "##files", &state.panes.file_pane_visible);
//   if (left_pane.begin(state.panes.file_pane_width)) {
//       // ... your pane content ...
//       left_pane.end();
//   }
//   ImGui::SameLine(0, 0);  // proceed to the next pane / splitter
//
// Note: the collapsed strip is still rendered (so transitions look smooth).
// The caller must call end() only when begin() returns true.

#include "imgui.h"
#include <string>
#include <string_view>

namespace gnc_viz {

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
    /// @param width  Current expanded width (from AppState.panes.*_width).
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
