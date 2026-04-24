#pragma once
/// @file annotation_tool.hpp
/// @brief IVisualizationTool that places text annotations on the plot.
/// @ingroup tools
#include "fastscope/interfaces.hpp"
#include <vector>
#include <string>

namespace fastscope {

/// @brief A single text annotation placed at a plot-space coordinate.
/// The callout bubble is offset from the anchor in pixel space so it
/// maintains a fixed visual distance regardless of zoom level.
struct Annotation {
    double      time;               ///< Anchor X in plot space.
    double      value;              ///< Anchor Y in plot space.
    std::string text;               ///< Label text.
    float       label_offset_x = 60.0f;  ///< Bubble centre pixel offset from anchor (X).
    float       label_offset_y = -80.0f; ///< Bubble centre pixel offset from anchor (Y).
};

/// @brief IVisualizationTool that places text annotations on the plot.
/// @details Left-click in the plot area to begin placing an annotation.
///          The user types a label; pressing Enter confirms placement.
///          Annotations are stored in AppState::annotations (not in the tool
///          instance) so they survive tool switching and can be serialised.
class AnnotationTool : public IVisualizationTool {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Annotation"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "annotation"; }
    [[nodiscard]] std::string_view icon() const noexcept override { return "A"; }

    void handle_input(AppState& state) override;
    void render_overlay(const AppState& state) override;
    void on_activate()   override {}
    void on_deactivate() override { m_placing = false; }

private:
    bool   m_placing     = false;
    double m_place_time  = 0.0;
    double m_place_value = 0.0;
    char   m_text_buf[256] = {};
};

} // namespace fastscope
