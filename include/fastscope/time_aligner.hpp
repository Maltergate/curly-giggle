#pragma once
/// @file time_aligner.hpp
/// @brief Aligns multiple SignalBuffers onto a common uniform time grid.
/// @ingroup data_layer

#include "fastscope/error.hpp"
#include "fastscope/signal_buffer.hpp"

#include <memory>
#include <span>
#include <vector>

namespace fastscope {

/// @brief Aligns multiple SignalBuffers onto a common uniform time grid.
/// @details Uses linear interpolation to resample all inputs onto a uniform grid
///          defined by their time overlap and the highest-frequency input.
class TimeAligner {
public:
    /// Align N signal buffers onto a common time grid using linear interpolation.
    ///
    /// Algorithm:
    ///   1. Common time range = intersection: [max(all starts), min(all ends)]
    ///      If intersection is empty, return ErrorCode::InvalidArgument.
    ///   2. Grid resolution = dt of the highest-frequency input
    ///      (i.e. smallest average sample interval).
    ///   3. Generate uniform grid from start to end at that resolution.
    ///   4. For each input: linearly interpolate all components onto the grid.
    ///
    /// If all inputs already share an identical time vector (same pointer or
    /// element-wise equality), returns copies that share the same time data
    /// without interpolation (fast path for single-simulation comparisons).
    ///
    /// @param inputs  Two or more loaded SignalBuffers. Empty or single-buffer
    ///                inputs are valid: single-buffer returns a copy unchanged.
    static fastscope::Result<std::vector<std::shared_ptr<SignalBuffer>>>
    align(std::span<const std::shared_ptr<SignalBuffer>> inputs);

    /// Build a uniform time grid over [t_start, t_end] with step dt.
    /// dt must be > 0. Used internally and exposed for testing.
    static std::vector<double>
    make_uniform_grid(double t_start, double t_end, double dt);

    /// Linear interpolation of a single component column from src onto dst_time.
    /// src_time and src_vals must have the same length.
    /// Out-of-range queries clamp to the boundary value.
    static std::vector<double>
    interpolate(std::span<const double> src_time,
                std::span<const double> src_vals,
                std::span<const double> dst_time);
};

} // namespace fastscope
