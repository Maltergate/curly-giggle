#pragma once

// ── ISignalOperation — abstract interface for signal transformations ───────────
//
// This file re-exports gnc_viz::ISignalOperation from interfaces.hpp and adds
// the concrete built-in operations.
//
// Built-in operations (Phase 3):
//   AddOp         — element-wise addition of two aligned scalar signals
//   SubtractOp    — element-wise subtraction
//   MultiplyOp    — element-wise multiplication
//   ScaleOp       — multiply each sample by a scalar constant
//   MagnitudeOp   — √(x₀² + x₁² + … + xₙ₋₁²) from N scalar signals
//
// All operations use TimeAligner (Phase 2) to align inputs before computing.
// For Phase 3, a stub alignment (no-op, assumes same time base) is used.

#include "gnc_viz/interfaces.hpp"
#include "gnc_viz/signal_buffer.hpp"

#include <span>

namespace gnc_viz {

// ── AddOp ─────────────────────────────────────────────────────────────────────

class AddOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "Add"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "add"; }
    [[nodiscard]] int              input_count() const noexcept override { return 2; }

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

// ── SubtractOp ────────────────────────────────────────────────────────────────

class SubtractOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "Subtract"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "subtract"; }
    [[nodiscard]] int              input_count() const noexcept override { return 2; }

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

// ── MultiplyOp ────────────────────────────────────────────────────────────────

class MultiplyOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "Multiply"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "multiply"; }
    [[nodiscard]] int              input_count() const noexcept override { return 2; }

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

// ── ScaleOp ───────────────────────────────────────────────────────────────────
// Multiplies all values by a constant factor.
// The factor is set before calling execute().

class ScaleOp final : public ISignalOperation {
public:
    double scale_factor = 1.0;

    [[nodiscard]] std::string_view name()        const noexcept override { return "Scale"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "scale"; }
    [[nodiscard]] int              input_count() const noexcept override { return 1; }

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

// ── MagnitudeOp ───────────────────────────────────────────────────────────────
// √(x₀² + x₁² + … + xₙ₋₁²) from N scalar input signals.

class MagnitudeOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "Magnitude"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "magnitude"; }
    [[nodiscard]] int              input_count() const noexcept override { return -1; }  // variadic

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

} // namespace gnc_viz
