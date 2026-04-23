#pragma once
/// @file signal_metadata.hpp
/// @brief Lightweight descriptor for an HDF5 signal dataset.
/// @ingroup data_layer

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gnc_viz {

/// @brief Numeric element type of an HDF5 dataset.
// ── DataType enumeration ───────────────────────────────────────────────────────

enum class DataType : uint8_t {
    Unknown  = 0,
    Float32,
    Float64,
    Int32,
    Int64,
    UInt32,
    UInt64,
};

/// Returns a human-readable string for a DataType value.
[[nodiscard]] const char* data_type_name(DataType dt) noexcept;

// ── SignalMetadata ─────────────────────────────────────────────────────────────

/// @brief Lightweight descriptor for one HDF5 signal dataset.
/// @details Cheap to copy; carries no sample data.
///          Populated by HDF5Reader::enumerate_signals(). shape[0] is the sample count.
struct SignalMetadata {
    /// Basename of the signal (last path component), e.g. "quaternion_body".
    std::string name;

    /// Full HDF5 dataset path, e.g. "/attitude/quaternion_body".
    std::string h5_path;

    /// Physical units, e.g. "m", "deg/s", "rad", "" for dimensionless.
    std::string units;

    /// Dataset element type.
    DataType dtype = DataType::Unknown;

    /// Shape of the dataset.  shape[0] is usually the sample count.
    /// e.g. {10000, 4} for a 10000-sample quaternion.
    std::vector<std::size_t> shape;

    /// Path to the corresponding time axis dataset.
    /// Empty → use the global "/time" axis.
    std::string time_path;

    /// Simulation ID (set by SimulationFile when it enumerates its signals).
    std::string sim_id;

    // ── Convenience helpers ────────────────────────────────────────────────────

    /// Number of time samples (shape[0], or 0 if shape is empty).
    [[nodiscard]] std::size_t sample_count() const noexcept
    {
        return shape.empty() ? 0 : shape[0];
    }

    /// Number of components per sample (product of shape[1..], or 1 for scalar).
    [[nodiscard]] std::size_t component_count() const noexcept
    {
        std::size_t n = 1;
        for (std::size_t i = 1; i < shape.size(); ++i) n *= shape[i];
        return n;
    }

    /// True if the signal is a scalar time series (shape == {N}).
    [[nodiscard]] bool is_scalar() const noexcept { return shape.size() == 1; }

    /// True if the signal is a vector time series (shape == {N, K}, K>1).
    [[nodiscard]] bool is_vector() const noexcept
    {
        return shape.size() == 2 && shape[1] > 1;
    }

    /// Unique key for display and deduplication: "<sim_id>/<h5_path>".
    [[nodiscard]] std::string unique_key() const
    {
        return sim_id + "/" + h5_path;
    }
};

} // namespace gnc_viz
