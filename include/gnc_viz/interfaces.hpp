#pragma once
/// @file interfaces.hpp
/// @brief Core pluggable interfaces: IPlotType, ISignalOperation, IVisualizationTool.
/// @defgroup core_interfaces Core Interfaces
/// @brief Extension points for plot types, signal operations, and visualization tools.

#include "gnc_viz/error.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace gnc_viz {

// ── Forward declarations ───────────────────────────────────────────────────────
class SignalBuffer;   // Phase 2 — defined in signal_buffer.hpp
struct AppState;

// ── SignalBuffer stub (placeholder until Phase 2) ──────────────────────────────
//
// Real definition lives in signal_buffer.hpp once Phase 2 is implemented.
// Interfaces need only the type name; callers use shared_ptr<SignalBuffer>.

/// @brief Abstract interface for a plot type (Time Series, Trajectory 2D, Ground Track, …).
/// @details Implementations are registered with PlotRegistry and activated via PlotEngine.
///          The render() method is called every frame inside the active ImGui child window.
class IPlotType {
public:
    virtual ~IPlotType() = default;

    /// Human-readable name shown in the plot-type selector UI (e.g. "Time Series").
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// Short identifier used as registry key (e.g. "timeseries").
    [[nodiscard]] virtual std::string_view id() const noexcept = 0;

    /// Render the plot for the current frame.
    /// @param state  Full application state. Mutable so plots can lazy-load buffers.
    /// @param width  Available pixel width for the plot.
    /// @param height Available pixel height for the plot.
    virtual void render(AppState& state, float width, float height) = 0;

    /// Called when this plot type becomes active.  Reset zoom state, etc.
    virtual void on_activate(AppState& /*state*/) {}

    /// Called when this plot type is deactivated (another type selected).
    virtual void on_deactivate() {}
};

/// @brief Abstract interface for a signal transformation operation.
/// @details Transforms N input SignalBuffers into one output SignalBuffer.
///          Implementations: AddOp, SubtractOp, MultiplyOp, ScaleOp, MagnitudeOp.
class ISignalOperation {
public:
    virtual ~ISignalOperation() = default;

    /// Human-readable name (e.g. "Add", "Magnitude").
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// Short registry key (e.g. "add", "magnitude").
    [[nodiscard]] virtual std::string_view id() const noexcept = 0;

    /// Number of required inputs.  Returns -1 for variadic (≥2 inputs).
    [[nodiscard]] virtual int input_count() const noexcept = 0;

    /// Execute the operation.
    /// @param inputs  Spans of time + value pairs.  Caller guarantees alignment.
    /// @returns       A new SignalBuffer, or an error describing the failure.
    virtual gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) = 0;
};

/// @brief Abstract interface for an interactive overlay tool rendered on the plot.
/// @details Implementations: AnnotationTool, RulerTool.
///          handle_input() is called when the plot is hovered; render_overlay() every frame.
class IVisualizationTool {
public:
    virtual ~IVisualizationTool() = default;

    /// Human-readable name (e.g. "Annotation", "Ruler").
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// Short registry key (e.g. "annotation", "ruler").
    [[nodiscard]] virtual std::string_view id() const noexcept = 0;

    /// Icon character shown in the toolbar (single UTF-8 glyph or ASCII).
    [[nodiscard]] virtual std::string_view icon() const noexcept = 0;

    /// Process mouse/keyboard input.  Called only when the plot area is hovered.
    /// @param state  Mutable — tool may add annotations to state.
    virtual void handle_input(AppState& state) = 0;

    /// Draw overlays via ImDrawList.  Called every frame (even when not hovered).
    virtual void render_overlay(const AppState& state) = 0;

    /// Called when the tool is activated (toolbar button pressed).
    virtual void on_activate() {}

    /// Called when another tool is activated (this one deactivated).
    virtual void on_deactivate() {}
};

/// @brief Concept: T must derive from IPlotType and be default-constructible.
template<typename T>
concept PlotTypeConcept =
    std::derived_from<T, IPlotType> &&
    std::default_initializable<T>;

/// @brief Concept: T must derive from ISignalOperation and be default-constructible.
template<typename T>
concept SignalOperationConcept =
    std::derived_from<T, ISignalOperation> &&
    std::default_initializable<T>;

/// @brief Concept: T must derive from IVisualizationTool and be default-constructible.
template<typename T>
concept VisualizationToolConcept =
    std::derived_from<T, IVisualizationTool> &&
    std::default_initializable<T>;

} // namespace gnc_viz
