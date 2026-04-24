#pragma once
/// @file plot_engine.hpp
/// @brief Owns the active IPlotType and PlotRegistry; dispatches render calls.
/// @defgroup plot_system Plot System
/// @brief Plot types, axis management, and the plot engine dispatcher.

#include "fastscope/registry.hpp"
#include "fastscope/interfaces.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace fastscope {

struct AppState;

/// @brief Owns the active IPlotType and PlotRegistry; dispatches render calls.
/// @details Registers all built-in plot types on construction (TimeSeriesPlot,
///          Trajectory2DPlot, GroundTrackPlot). Manages activation lifecycle.
class PlotEngine {
public:
    /// @brief Construct and register all built-in plot types.
    PlotEngine();   // registers TimeSeriesPlot, Trajectory2DPlot, GroundTrackPlot

    /// @brief Render the active plot type.
    /// @param state  Application state (mutable for lazy loading).
    /// @param width  Available pixel width.
    /// @param height Available pixel height.
    void render(AppState& state, float width, float height);

    /// @brief Switch active plot type; calls on_deactivate/on_activate.
    /// @param type_id Registry key of the plot type to activate.
    /// @param state   Application state passed to on_activate().
    void switch_to(std::string_view type_id, AppState& state);

    /// @brief Returns the registry key of the currently active plot type.
    [[nodiscard]] std::string_view current_type_id() const noexcept;

    /// @brief Returns all registered plot type keys in insertion order.
    [[nodiscard]] const std::vector<std::string>& available_types() const noexcept;

    /// @brief Trigger auto-fit on both X and all Y axes next frame.
    void fit_to_data();
    /// @brief Trigger auto-fit on X axis only next frame.
    void fit_x_only();
    /// @brief Trigger auto-fit on all Y axes only next frame.
    void fit_y_only();

    /// @brief Direct access to the active IPlotType (may be nullptr).
    IPlotType*       current_type() noexcept;
    /// @brief Const access to the active IPlotType (may be nullptr).
    const IPlotType* current_type() const noexcept;

private:
    PlotRegistry                 m_registry;
    std::string                  m_current_id;
    std::unique_ptr<IPlotType>   m_current;
};

} // namespace fastscope
