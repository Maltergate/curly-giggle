# Prompt: Add a New Visualization Tool to FastScope

Use this prompt when you need to implement an interactive overlay tool on the plot
(e.g. a ruler, annotation, crosshair, data cursor, region-select, etc.).

---

## Context

Visualization tools implement `IVisualizationTool` (`include/fastscope/interfaces.hpp`).
They render overlays on top of the active plot and handle mouse/keyboard input.
They are registered in `ToolManager` (`src/app/tool_manager.cpp`).
Existing tools: `AnnotationTool` (`annotation_tool.cpp`), `RulerTool` (`ruler_tool.cpp`).

Key files to read before starting:
- `include/fastscope/interfaces.hpp` — IVisualizationTool interface
- `include/fastscope/tool_manager.hpp` — ToolManager, toolbar rendering
- `src/app/ruler_tool.cpp` — reference implementation
- `src/app/annotation_tool.cpp` — reference implementation
- `include/fastscope/app_state.hpp` — AppState structure

---

## Step 1 — Create the header

File: `include/fastscope/my_tool.hpp`

```cpp
#pragma once
/// @file my_tool.hpp
/// @brief [One-line description].
#include "fastscope/interfaces.hpp"
#include <vector>

namespace fastscope {

/// @brief [Full description of what the tool does and how the user interacts with it].
class MyTool final : public IVisualizationTool {
public:
    /// @brief Human-readable name shown in the toolbar tooltip.
    [[nodiscard]] std::string_view name() const noexcept override { return "My Tool"; }

    /// @brief Unique registry key.
    [[nodiscard]] std::string_view id() const noexcept override { return "mytool"; }

    /// @brief Single-character icon shown in the toolbar button.
    [[nodiscard]] std::string_view icon() const noexcept override { return "M"; }

    void handle_input(AppState& state) override;
    void render_overlay(const AppState& state) override;
    void on_activate() override;
    void on_deactivate() override;

private:
    // Per-tool state — e.g. click points, labels, visibility flags
    bool m_active = false;
};

} // namespace fastscope
```

---

## Step 2 — Create the source file

File: `src/app/my_tool.cpp`

```cpp
#include "fastscope/my_tool.hpp"
#include "fastscope/app_state.hpp"
#include <imgui.h>
#include <implot.h>

namespace fastscope {

void MyTool::on_activate() {
    m_active = false;
    // Reset any accumulated state
}

void MyTool::on_deactivate() {
    // Nothing to clean up unless you own ImGui IDs
}

void MyTool::handle_input(AppState& state) {
    // Called every frame when this tool is active AND the plot is hovered.
    // ImPlot::IsPlotHovered() is already true when this is called.
    //
    // Use ImPlot::GetPlotMousePos() to get data-space coordinates.
    // Use ImGui::IsMouseClicked(0) for left-click.
    //
    // Example: record a point on left-click
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        auto pos = ImPlot::GetPlotMousePos();
        // store pos...
    }
}

void MyTool::render_overlay(const AppState& state) {
    // Called every frame inside an active ImPlot::BeginPlot() / EndPlot() block.
    // Use ImPlot::PlotText(), ImPlot::PlotInfLines(), ImDrawList, etc.
    //
    // IMPORTANT: Never allocate vectors here. Use member variables.
    //
    // Example: draw a vertical line
    // ImPlot::PlotInfLines("##mytool_line", &m_x, 1);
}

} // namespace fastscope
```

**Rules:**
- `handle_input()` is called BEFORE `render_overlay()` in the same frame.
- Do not call `ImPlot::BeginPlot()` / `EndPlot()` inside these methods — you are already inside one.
- Do not allocate heap memory in `render_overlay()`.
- Use ImGui IDs that are unique to this tool to avoid conflicts with other tools.

---

## Step 3 — Register the tool

File: `src/app/tool_manager.cpp`

```cpp
#include "fastscope/my_tool.hpp"   // ← add this include

ToolManager::ToolManager() {
    m_registry.register_type<AnnotationTool>("annotation");
    m_registry.register_type<RulerTool>("ruler");
    m_registry.register_type<MyTool>("mytool");   // ← add this line
}
```

---

## Step 4 — Add to CMakeLists.txt

File: `src/app/CMakeLists.txt`

```cmake
add_executable(fastscope
    # ... existing sources ...
    my_tool.cpp    # ← add this line
)
```

---

## Step 5 — (Optional) Serialize tool state

If the tool creates annotations or measurements that should survive session save/restore,
add serialization to `src/lib/session_serializer.cpp` (Phase 14 feature).
For now, tools can lose state on restart — add a TODO comment.

---

## Acceptance Criteria

Before committing:
1. `cmake --build build -j$(sysctl -n hw.ncpu)` — zero errors
2. `./build/bin/fastscope_tests` — all tests pass
3. Tool appears in toolbar (icon button), activates on click, renders overlay
4. Tool deactivates cleanly when another tool is selected (no stale state)
5. FPS stays at 60 with the tool active (no allocs in render_overlay)
6. Add `///` Doxygen comments to all public members in the header
