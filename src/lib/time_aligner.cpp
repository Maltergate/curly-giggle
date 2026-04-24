#include "fastscope/time_aligner.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace fastscope {

std::vector<double>
TimeAligner::make_uniform_grid(double t_start, double t_end, double dt)
{
    std::vector<double> grid;
    if (dt <= 0.0 || t_start > t_end) return grid;

    const std::size_t n = static_cast<std::size_t>(
        std::floor((t_end - t_start) / dt + 1e-9)) + 1;
    grid.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        double t = t_start + static_cast<double>(i) * dt;
        if (t > t_end + dt * 1e-9) break;
        grid.push_back(t);
    }
    return grid;
}

std::vector<double>
TimeAligner::interpolate(std::span<const double> src_time,
                         std::span<const double> src_vals,
                         std::span<const double> dst_time)
{
    std::vector<double> out;
    out.reserve(dst_time.size());

    const std::size_t N = src_time.size();

    for (double t : dst_time) {
        if (N == 0) {
            out.push_back(0.0);
            continue;
        }
        if (t <= src_time[0]) {
            out.push_back(src_vals[0]);
            continue;
        }
        if (t >= src_time[N - 1]) {
            out.push_back(src_vals[N - 1]);
            continue;
        }
        // Binary search for the first element >= t
        auto it = std::lower_bound(src_time.begin(), src_time.end(), t);
        const std::size_t hi = static_cast<std::size_t>(it - src_time.begin());
        const std::size_t lo = hi - 1;

        const double t0 = src_time[lo], t1 = src_time[hi];
        const double v0 = src_vals[lo],  v1 = src_vals[hi];
        const double alpha = (t - t0) / (t1 - t0);
        out.push_back(v0 + alpha * (v1 - v0));
    }
    return out;
}

fastscope::Result<std::vector<std::shared_ptr<SignalBuffer>>>
TimeAligner::align(std::span<const std::shared_ptr<SignalBuffer>> inputs)
{
    if (inputs.empty()) {
        return std::vector<std::shared_ptr<SignalBuffer>>{};
    }

    // Single buffer: return a copy unchanged
    if (inputs.size() == 1) {
        const auto& src = inputs[0];
        auto copy = SignalBuffer::make_vector(
            src->metadata(),
            std::vector<double>(src->time().begin(), src->time().end()),
            std::vector<double>(src->values().begin(), src->values().end()),
            src->n_components());
        return std::vector<std::shared_ptr<SignalBuffer>>{std::move(copy)};
    }

    // Fast path: check if all inputs share the same time vector
    bool all_same_time = true;
    const auto first_time = inputs[0]->time();
    for (std::size_t i = 1; i < inputs.size(); ++i) {
        const auto t = inputs[i]->time();
        if (t.data() == first_time.data()) continue; // identical pointer
        if (t.size() != first_time.size()) { all_same_time = false; break; }
        for (std::size_t j = 0; j < t.size(); ++j) {
            if (t[j] != first_time[j]) { all_same_time = false; break; }
        }
        if (!all_same_time) break;
    }

    if (all_same_time) {
        std::vector<std::shared_ptr<SignalBuffer>> result;
        result.reserve(inputs.size());
        for (const auto& src : inputs) {
            result.push_back(SignalBuffer::make_vector(
                src->metadata(),
                std::vector<double>(src->time().begin(), src->time().end()),
                std::vector<double>(src->values().begin(), src->values().end()),
                src->n_components()));
        }
        return result;
    }

    // Compute the intersection of all time ranges
    double t_start = inputs[0]->time().front();
    double t_end   = inputs[0]->time().back();
    for (const auto& buf : inputs) {
        t_start = std::max(t_start, buf->time().front());
        t_end   = std::min(t_end,   buf->time().back());
    }

    if (t_start >= t_end) {
        return fastscope::make_error<std::vector<std::shared_ptr<SignalBuffer>>>(
            fastscope::ErrorCode::InvalidArgument,
            "Time ranges of inputs do not overlap");
    }

    // Find the highest-frequency input (smallest average sample interval)
    double best_dt = std::numeric_limits<double>::max();
    for (const auto& buf : inputs) {
        const auto t = buf->time();
        if (t.size() < 2) continue;
        const double avg_dt =
            (t.back() - t.front()) / static_cast<double>(t.size() - 1);
        if (avg_dt > 0.0)
            best_dt = std::min(best_dt, avg_dt);
    }

    if (best_dt <= 0.0 || best_dt == std::numeric_limits<double>::max()) {
        return fastscope::make_error<std::vector<std::shared_ptr<SignalBuffer>>>(
            fastscope::ErrorCode::InvalidArgument,
            "Cannot determine a valid time step from inputs");
    }

    const auto grid = make_uniform_grid(t_start, t_end, best_dt);

    // Interpolate each buffer onto the common grid
    std::vector<std::shared_ptr<SignalBuffer>> result;
    result.reserve(inputs.size());

    for (const auto& src : inputs) {
        const std::size_t K        = src->n_components();
        const auto        src_time = src->time();
        std::vector<double> new_values(grid.size() * K, 0.0);

        for (std::size_t k = 0; k < K; ++k) {
            std::vector<double> comp_vals;
            comp_vals.reserve(src->sample_count());
            for (std::size_t i = 0; i < src->sample_count(); ++i)
                comp_vals.push_back(src->at(i, k));

            const auto interp = interpolate(src_time, comp_vals, grid);
            for (std::size_t i = 0; i < grid.size(); ++i)
                new_values[i * K + k] = interp[i];
        }

        result.push_back(SignalBuffer::make_vector(
            src->metadata(),
            std::vector<double>(grid),
            std::move(new_values),
            K));
    }

    return result;
}

} // namespace fastscope
