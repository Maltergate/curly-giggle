#pragma once
/// @file annotation_tool.hpp
/// @brief IVisualizationTool that places text annotations on the plot.
/// @ingroup tools
#include "fastscope/interfaces.hpp"
#include <vector>
#include <string>

namespace fastscope {

/// @brief A single text annotation placed at a plot-space coordinate.
struct Annotation {
    double      time;           ///< X position in plot space.
    double      value;          ///< Y position in plot space.
    std::string text;           ///< Label text.
    bool        has_arrow = false; ///< Whether to draw an arrow to the point.
};

/// @brief IVisualizationTool that places text annotations on the plot.
/// @details Left-click in the plot area to begin placing an annotation.
///          The user types a label; pressing Enter confirms placement.
class AnnotationTool : public IVisualizationTool {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Annotation"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "annotation"; }
    [[nodiscard]] std::string_view icon() const noexcept override { return "A"; }

    void handle_input(AppState& state) override;
    void render_overlay(const AppState& state) override;
    void on_activate()   override {}
    void on_deactivate() override { m_placing = false; }

    [[nodiscard]] const std::vector<Annotation>& annotations() const noexcept;
    void clear_annotations();

private:
    std::vector<Annotation> m_annotations;
    bool   m_placing     = false;
    double m_place_time  = 0.0;
    double m_place_value = 0.0;
    char   m_text_buf[256] = {};
};

} // namespace fastscope
