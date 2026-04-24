#pragma once
/// @file ruler_tool.hpp
/// @brief IVisualizationTool for two-point distance measurement.
/// @ingroup tools
#include "fastscope/interfaces.hpp"

namespace fastscope {

/// @brief IVisualizationTool for two-point distance measurement.
/// @details Left-click sets the start point; right-click sets the end point.
///          Renders a line with delta-time and delta-value annotations.
class RulerTool : public IVisualizationTool {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Ruler"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "ruler"; }
    [[nodiscard]] std::string_view icon() const noexcept override { return "R"; }

    void handle_input(AppState& state) override;
    void render_overlay(const AppState& state) override;
    void on_activate()   override { m_has_start = false; m_has_end = false; }
    void on_deactivate() override { m_has_start = false; m_has_end = false; }

private:
    bool   m_has_start = false;
    bool   m_has_end   = false;
    double m_start_t = 0.0, m_start_v = 0.0;
    double m_end_t   = 0.0, m_end_v   = 0.0;
};

} // namespace fastscope
