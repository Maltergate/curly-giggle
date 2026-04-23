#pragma once

// ── HDF5Reader — enumerate and load HDF5 datasets ─────────────────────────────
//
// Opens an HDF5 file and provides two operations:
//   1. enumerate_signals() — recursively walks all datasets, returns metadata
//      WITHOUT loading any data.  Fast; suitable for populating the signal tree.
//   2. load_signal()        — reads one dataset into a SignalBuffer.
//
// Thread safety:
//   - enumerate_signals() and load_signal() may be called from any thread,
//     but NOT concurrently.  The caller is responsible for serialising access.
//   - Intended usage: spawn a std::jthread; call from that thread only.
//
// Error handling:
//   All public methods return gnc::Result<T> — never throw.
//
// HDF5 naming convention expected by this reader:
//   - The file should contain a "/time" dataset (1-D, float64) as global time axis.
//   - Other datasets are signals.  Their shape is {N} (scalar) or {N, K} (vector).
//   - Each dataset may have a "units" attribute (string).

#include "gnc_viz/error.hpp"
#include "gnc_viz/signal_metadata.hpp"
#include "gnc_viz/signal_buffer.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace gnc_viz {

class HDF5Reader {
public:
    HDF5Reader();
    ~HDF5Reader();

    // non-copyable (HDF5 handles are not copyable)
    HDF5Reader(const HDF5Reader&)            = delete;
    HDF5Reader& operator=(const HDF5Reader&) = delete;

    // movable
    HDF5Reader(HDF5Reader&&) noexcept;
    HDF5Reader& operator=(HDF5Reader&&) noexcept;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /// Open an HDF5 file.  Returns an error if the file cannot be opened.
    [[nodiscard]] gnc::VoidResult open(const std::filesystem::path& path);

    /// Close the file and release all HDF5 handles.
    void close() noexcept;

    /// True if a file is currently open.
    [[nodiscard]] bool is_open() const noexcept;

    // ── Signal enumeration ────────────────────────────────────────────────────

    /// Recursively enumerate all datasets in the file.
    /// The "/time" dataset is excluded from the result.
    /// @param sim_id   Tag set on each SignalMetadata::sim_id field.
    [[nodiscard]] gnc::Result<std::vector<SignalMetadata>>
    enumerate_signals(const std::string& sim_id = "") const;

    // ── Signal loading ────────────────────────────────────────────────────────

    /// Load a signal dataset into a SignalBuffer.
    /// Also loads the time axis referenced by meta.time_path (default: "/time").
    [[nodiscard]] gnc::Result<std::shared_ptr<SignalBuffer>>
    load_signal(const SignalMetadata& meta) const;

    // ── File info ─────────────────────────────────────────────────────────────

    [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace gnc_viz
