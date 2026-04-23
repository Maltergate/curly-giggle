#pragma once

// ── Core pluggable interfaces ──────────────────────────────────────────────────
//
// These three pure-abstract classes are the extension points of GNC Viz.
// Adding a new plot type / signal operation / tool = implement one of these
// interfaces and register it with the corresponding Registry<T>.
//
// Rules:
//  1. All methods use smart pointers or references — no raw owning pointers.
//  2. Implementations must be default-constructible (required by Registry<T>).
//  3. No ImGui headers here — interfaces are pure C++.

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

// ── IPlotType ─────────────────────────────────────────────────────────────────
//
// A plot type knows how to render one or more signals in ImPlot.
// Implementations: TimeSeriesPlot, Trajectory2DPlot, GroundTrackPlot, ...
//
// Render() is called every frame.  The implementation calls ImPlot::BeginPlot()
// inside the already-open ImGui child window provided by PlotEngine.

class IPlotType {
public:
    virtual ~IPlotType() = default;

    /// Human-readable name shown in the plot-type selector UI (e.g. "Time Series").
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// Short identifier used as registry key (e.g. "timeseries").
    [[nodiscard]] virtual std::string_view id() const noexcept = 0;

    /// Render the plot for the current frame.
    /// @param state  Full application state (read-only for most plot types).
    /// @param size   Available pixel region for the plot (pass to ImPlot::BeginPlot).
    virtual void render(const AppState& state, float width, float height) = 0;

    /// Called when this plot type becomes active.  Reset zoom state, etc.
    virtual void on_activate(const AppState& /*state*/) {}

    /// Called when this plot type is deactivated (another type selected).
    virtual void on_deactivate() {}
};

// ── ISignalOperation ──────────────────────────────────────────────────────────
//
// Transforms N input signal buffers into one output buffer.
// Implementations: AddOp, SubtractOp, MultiplyOp, ScaleOp, MagnitudeOp, ...

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

// ── IVisualizationTool ────────────────────────────────────────────────────────
//
// An interactive overlay tool rendered on top of the active plot.
// Implementations: AnnotationTool, RulerTool, ...
//
// The tool system calls these methods in order each frame:
//   1. handle_input()  — process mouse / keyboard while the plot is hovered
//   2. render_overlay() — draw ImDrawList annotations on top of the plot

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

// ── C++20 concept constraints used by Registry<T> ────────────────────────────
//
// Each registry constraint ensures:
//  1. The type derives from the expected interface.
//  2. The type is default-constructible (registry creates instances via T()).

template<typename T>
concept PlotTypeConcept =
    std::derived_from<T, IPlotType> &&
    std::default_initializable<T>;

template<typename T>
concept SignalOperationConcept =
    std::derived_from<T, ISignalOperation> &&
    std::default_initializable<T>;

template<typename T>
concept VisualizationToolConcept =
    std::derived_from<T, IVisualizationTool> &&
    std::default_initializable<T>;

} // namespace gnc_viz
