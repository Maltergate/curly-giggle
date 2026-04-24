#pragma once
/// @file signal_ops.hpp
/// @brief Built-in ISignalOperation implementations: Add, Subtract, Multiply, Scale, Magnitude.
/// @defgroup signal_ops Signal Operations
/// @brief Derived signal computation operations.

#include "gnc_viz/interfaces.hpp"
#include "gnc_viz/signal_buffer.hpp"

#include <span>

namespace gnc_viz {

// ── AddOp ─────────────────────────────────────────────────────────────────────

/// @brief Element-wise addition of two aligned scalar signals.
class AddOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "Add"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "add"; }
    [[nodiscard]] int              input_count() const noexcept override { return 2; }

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

// ── SubtractOp ────────────────────────────────────────────────────────────────

/// @brief Element-wise subtraction of two aligned scalar signals (inputs[0] - inputs[1]).
class SubtractOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "Subtract"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "subtract"; }
    [[nodiscard]] int              input_count() const noexcept override { return 2; }

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

// ── MultiplyOp ────────────────────────────────────────────────────────────────

/// @brief Element-wise multiplication of two aligned scalar signals.
class MultiplyOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "Multiply"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "multiply"; }
    [[nodiscard]] int              input_count() const noexcept override { return 2; }

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

// ── ScaleOp ───────────────────────────────────────────────────────────────────

/// @brief Multiplies all values by a constant scale_factor.
/// @details The factor is set on the struct before calling execute().
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

/// @brief Computes √(x₀² + x₁² + … + xₙ₋₁²) from N scalar input signals (variadic).
class MagnitudeOp final : public ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "Magnitude"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "magnitude"; }
    [[nodiscard]] int              input_count() const noexcept override { return -1; }  // variadic

    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};

} // namespace gnc_viz
