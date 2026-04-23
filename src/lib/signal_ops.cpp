#include "gnc_viz/signal_ops.hpp"
#include <cmath>
#include <format>

namespace gnc_viz {

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Validate that inputs has exactly the required count (or ≥ 2 for variadic).
static gnc::VoidResult validate_inputs(
    std::span<const std::shared_ptr<SignalBuffer>> inputs,
    int required_count)
{
    if (inputs.empty() || inputs[0] == nullptr)
        return gnc::make_void_error(gnc::ErrorCode::InvalidArgument,
                                    "No input signals provided");

    if (required_count > 0 && static_cast<int>(inputs.size()) != required_count)
        return gnc::make_void_error(
            gnc::ErrorCode::InvalidArgument,
            std::format("Expected {} inputs, got {}", required_count, inputs.size()));

    if (required_count == -1 && inputs.size() < 2)
        return gnc::make_void_error(gnc::ErrorCode::InvalidArgument,
                                    "Variadic op requires at least 2 inputs");

    // All inputs must have the same sample count for the no-aligner stub
    const std::size_t N = inputs[0]->sample_count();
    for (std::size_t i = 1; i < inputs.size(); ++i) {
        if (inputs[i] == nullptr)
            return gnc::make_void_error(gnc::ErrorCode::InvalidArgument,
                                        std::format("Input {} is null", i));
        if (inputs[i]->sample_count() != N)
            return gnc::make_void_error(
                gnc::ErrorCode::InvalidArgument,
                std::format("Input sample count mismatch: {} vs {}",
                            N, inputs[i]->sample_count()));
        if (inputs[i]->n_components() != 1)
            return gnc::make_void_error(
                gnc::ErrorCode::InvalidArgument,
                "Signal operations require scalar (1-component) inputs");
    }
    if (inputs[0]->n_components() != 1)
        return gnc::make_void_error(gnc::ErrorCode::InvalidArgument,
                                    "Signal operations require scalar inputs");
    return {};
}

/// Build output metadata for a binary op result.
static SignalMetadata result_meta(const SignalMetadata& a, const std::string& op_name)
{
    SignalMetadata m;
    m.name    = op_name + "(" + a.name + ")";
    m.h5_path = "";       // derived — not from HDF5
    m.dtype   = DataType::Float64;
    m.shape   = a.shape;
    m.sim_id  = a.sim_id;
    return m;
}

// ── AddOp ─────────────────────────────────────────────────────────────────────

gnc::Result<std::shared_ptr<SignalBuffer>>
AddOp::execute(std::span<const std::shared_ptr<SignalBuffer>> inputs)
{
    if (auto v = validate_inputs(inputs, 2); !v) return gnc::make_error<std::shared_ptr<SignalBuffer>>(v.error().code, v.error().message);

    const auto& a = inputs[0];
    const auto& b = inputs[1];
    const std::size_t N = a->sample_count();

    std::vector<double> out(N);
    for (std::size_t i = 0; i < N; ++i)
        out[i] = a->at(i) + b->at(i);

    auto meta = result_meta(a->metadata(), "add");
    return SignalBuffer::make_scalar(std::move(meta),
                                     std::vector<double>(a->time().begin(), a->time().end()),
                                     std::move(out));
}

// ── SubtractOp ────────────────────────────────────────────────────────────────

gnc::Result<std::shared_ptr<SignalBuffer>>
SubtractOp::execute(std::span<const std::shared_ptr<SignalBuffer>> inputs)
{
    if (auto v = validate_inputs(inputs, 2); !v) return gnc::make_error<std::shared_ptr<SignalBuffer>>(v.error().code, v.error().message);

    const auto& a = inputs[0];
    const auto& b = inputs[1];
    const std::size_t N = a->sample_count();

    std::vector<double> out(N);
    for (std::size_t i = 0; i < N; ++i)
        out[i] = a->at(i) - b->at(i);

    auto meta = result_meta(a->metadata(), "sub");
    return SignalBuffer::make_scalar(std::move(meta),
                                     std::vector<double>(a->time().begin(), a->time().end()),
                                     std::move(out));
}

// ── MultiplyOp ────────────────────────────────────────────────────────────────

gnc::Result<std::shared_ptr<SignalBuffer>>
MultiplyOp::execute(std::span<const std::shared_ptr<SignalBuffer>> inputs)
{
    if (auto v = validate_inputs(inputs, 2); !v) return gnc::make_error<std::shared_ptr<SignalBuffer>>(v.error().code, v.error().message);

    const auto& a = inputs[0];
    const auto& b = inputs[1];
    const std::size_t N = a->sample_count();

    std::vector<double> out(N);
    for (std::size_t i = 0; i < N; ++i)
        out[i] = a->at(i) * b->at(i);

    auto meta = result_meta(a->metadata(), "mul");
    return SignalBuffer::make_scalar(std::move(meta),
                                     std::vector<double>(a->time().begin(), a->time().end()),
                                     std::move(out));
}

// ── ScaleOp ───────────────────────────────────────────────────────────────────

gnc::Result<std::shared_ptr<SignalBuffer>>
ScaleOp::execute(std::span<const std::shared_ptr<SignalBuffer>> inputs)
{
    if (auto v = validate_inputs(inputs, 1); !v) return gnc::make_error<std::shared_ptr<SignalBuffer>>(v.error().code, v.error().message);

    const auto& a = inputs[0];
    const std::size_t N = a->sample_count();

    std::vector<double> out(N);
    for (std::size_t i = 0; i < N; ++i)
        out[i] = a->at(i) * scale_factor;

    auto meta = result_meta(a->metadata(), "scale");
    return SignalBuffer::make_scalar(std::move(meta),
                                     std::vector<double>(a->time().begin(), a->time().end()),
                                     std::move(out));
}

// ── MagnitudeOp ───────────────────────────────────────────────────────────────

gnc::Result<std::shared_ptr<SignalBuffer>>
MagnitudeOp::execute(std::span<const std::shared_ptr<SignalBuffer>> inputs)
{
    if (auto v = validate_inputs(inputs, -1); !v) return gnc::make_error<std::shared_ptr<SignalBuffer>>(v.error().code, v.error().message);

    const std::size_t N = inputs[0]->sample_count();

    std::vector<double> out(N, 0.0);
    for (const auto& inp : inputs)
        for (std::size_t i = 0; i < N; ++i)
            out[i] += inp->at(i) * inp->at(i);
    for (auto& v : out) v = std::sqrt(v);

    auto meta = result_meta(inputs[0]->metadata(), "mag");
    return SignalBuffer::make_scalar(std::move(meta),
                                     std::vector<double>(inputs[0]->time().begin(),
                                                          inputs[0]->time().end()),
                                     std::move(out));
}

} // namespace gnc_viz
