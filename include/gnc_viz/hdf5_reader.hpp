#pragma once

// ── HDF5Reader — enumerate and load HDF5 datasets ─────────────────────────────
//
// Intentionally generic: makes NO assumptions about file layout.
//
//   1. enumerate_signals() — walks all datasets, returns metadata for every
//      dataset found (including any time axis).  The caller decides which
//      dataset is the time axis.
//
//   2. suggest_time_axes() — heuristic scan for datasets that look like a time
//      axis (1-D float, name contains "time"/"t").  Returns candidate paths
//      ordered by likelihood.  These are suggestions only; the user chooses.
//
//   3. load_signal()        — reads one dataset.  The time axis path must be
//      supplied explicitly via SignalMetadata::time_path, or synthesised from
//      sample index if left empty.
//
// Thread safety:
//   enumerate_signals(), suggest_time_axes(), and load_signal() may be called
//   from any thread, but NOT concurrently.
//
// Error handling:
//   All public methods return gnc::Result<T> — never throw.

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

    HDF5Reader(const HDF5Reader&)            = delete;
    HDF5Reader& operator=(const HDF5Reader&) = delete;
    HDF5Reader(HDF5Reader&&) noexcept;
    HDF5Reader& operator=(HDF5Reader&&) noexcept;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    [[nodiscard]] gnc::VoidResult open(const std::filesystem::path& path);
    void close() noexcept;
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;

    // ── Signal enumeration ────────────────────────────────────────────────────

    /// Enumerate every dataset in the file.  No dataset is excluded.
    /// @param sim_id  Tag written to each SignalMetadata::sim_id field.
    [[nodiscard]] gnc::Result<std::vector<SignalMetadata>>
    enumerate_signals(const std::string& sim_id = "") const;

    /// Return paths of datasets that look like a time axis, best-first:
    ///   - 1-D float dataset whose name contains "time" or equals "t"
    ///   - 2-D float dataset with shape (N, 1) and a time-like name
    /// The list may be empty if no candidates are found.
    [[nodiscard]] std::vector<std::string> suggest_time_axes() const;

    // ── Signal loading ────────────────────────────────────────────────────────

    /// Load a dataset into a SignalBuffer.
    ///
    /// Time axis resolution (in order):
    ///   1. meta.time_path if non-empty  → load that dataset as time
    ///   2. Otherwise                    → synthesise 0, 1, 2, … (sample index)
    ///
    /// Shape normalisation:
    ///   - (N,)   → scalar signal, 1 component
    ///   - (N, 1) → scalar signal, 1 component  (squeeze trailing 1)
    ///   - (N, K) → vector signal, K components  (K > 1)
    [[nodiscard]] gnc::Result<std::shared_ptr<SignalBuffer>>
    load_signal(const SignalMetadata& meta) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace gnc_viz

