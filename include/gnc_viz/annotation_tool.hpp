#pragma once
#include "gnc_viz/interfaces.hpp"
#include <vector>
#include <string>

namespace gnc_viz {

struct Annotation {
    double      time;           // X position in plot space
    double      value;          // Y position in plot space
    std::string text;           // label text
    bool        has_arrow = false;
};

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

} // namespace gnc_viz
