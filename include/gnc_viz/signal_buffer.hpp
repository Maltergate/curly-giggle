#pragma once

// ── SignalBuffer — on-demand time-series data container ───────────────────────
//
// Holds the raw numeric data for one signal from an HDF5 file.
// Owned via shared_ptr so multiple operations/plots share the same allocation.
//
// Memory model:
//  - Buffers are loaded lazily (on first access) by SimulationFile.
//  - The data can be evicted by resetting the shared_ptr (m_values goes empty).
//  - All downstream consumers hold weak_ptr or std::span (borrowed views).
//
// Layout:
//  - `time`   : 1-D array of sample timestamps in seconds  (length N)
//  - `values` : row-major 2-D array, rows = samples, cols = components
//               stored as flat vector of length N * n_components
//               e.g. a quaternion with N=5000 samples: values.size() = 20000
//               values[i * 4 + k] = sample i, component k
//
// Thread safety: none — load/evict from the owning SimulationFile only.

#include "gnc_viz/signal_metadata.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace gnc_viz {

class SignalBuffer {
public:
    // ── Construction ──────────────────────────────────────────────────────────

    SignalBuffer() = default;

    /// Construct directly from already-loaded data.
    /// @param metadata     Describes the signal (shape, dtype, units, …)
    /// @param time         Sample timestamps in seconds (length N)
    /// @param values       Flat row-major values array (length N × n_components)
    SignalBuffer(SignalMetadata metadata,
                 std::vector<double> time,
                 std::vector<double> values);

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] const SignalMetadata& metadata()    const noexcept { return m_meta; }
    [[nodiscard]] std::size_t           sample_count() const noexcept { return m_time.size(); }
    [[nodiscard]] std::size_t           n_components() const noexcept { return m_n_comp; }

    /// True if the buffer has been loaded (non-empty time vector).
    [[nodiscard]] bool is_loaded() const noexcept { return !m_time.empty(); }

    // ── Span accessors (zero-copy views) ──────────────────────────────────────

    /// View of all timestamps (length N).
    [[nodiscard]] std::span<const double> time() const noexcept
    {
        return {m_time.data(), m_time.size()};
    }

    /// View of the flat values array (length N × n_components).
    [[nodiscard]] std::span<const double> values() const noexcept
    {
        return {m_values.data(), m_values.size()};
    }

    /// View of a single component across all samples (stride = n_components).
    /// Returns a temporary vector (not zero-copy — strided access).
    /// Use values() + manual indexing for performance-critical paths.
    [[nodiscard]] std::vector<double> component(std::size_t comp_idx) const;

    /// Returns the value at sample i, component k.
    [[nodiscard]] double at(std::size_t sample_i, std::size_t comp_k = 0) const noexcept
    {
        assert(sample_i < m_time.size() && comp_k < m_n_comp);
        return m_values[sample_i * m_n_comp + comp_k];
    }

    // ── Eviction ──────────────────────────────────────────────────────────────

    /// Release the data vectors to free memory.
    /// After eviction, is_loaded() returns false; the SignalBuffer itself survives.
    void evict() noexcept
    {
        m_time.clear();
        m_time.shrink_to_fit();
        m_values.clear();
        m_values.shrink_to_fit();
    }

    // ── Factory helpers ───────────────────────────────────────────────────────

    /// Create a SignalBuffer from a scalar time series (n_components = 1).
    static std::shared_ptr<SignalBuffer>
    make_scalar(SignalMetadata meta,
                std::vector<double> time,
                std::vector<double> values);

    /// Create a SignalBuffer from a vector time series.
    static std::shared_ptr<SignalBuffer>
    make_vector(SignalMetadata meta,
                std::vector<double> time,
                std::vector<double> values,  // already flat row-major
                std::size_t n_components);

private:
    SignalMetadata     m_meta;
    std::vector<double> m_time;
    std::vector<double> m_values;
    std::size_t         m_n_comp = 1;
};

} // namespace gnc_viz
