#pragma once
/// @file hdf5_reader.hpp
/// @brief Low-level HDF5 dataset reader (enumerate + load). PIMPL pattern.
/// @ingroup data_layer

#include "fastscope/error.hpp"
#include "fastscope/signal_metadata.hpp"
#include "fastscope/signal_buffer.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fastscope {

/// @brief Low-level HDF5 dataset reader using a PIMPL pattern.
/// @details Enumerates datasets and loads them into SignalBuffers.
///          All public methods return fastscope::Result<T> — never throw.
///          Not thread-safe for concurrent calls.
class HDF5Reader {
public:
    HDF5Reader();
    ~HDF5Reader();

    HDF5Reader(const HDF5Reader&)            = delete;
    HDF5Reader& operator=(const HDF5Reader&) = delete;
    HDF5Reader(HDF5Reader&&) noexcept;
    HDF5Reader& operator=(HDF5Reader&&) noexcept;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    [[nodiscard]] fastscope::VoidResult open(const std::filesystem::path& path);
    void close() noexcept;
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;

    // ── Signal enumeration ────────────────────────────────────────────────────

    /// Enumerate every dataset in the file.  No dataset is excluded.
    /// @param sim_id  Tag written to each SignalMetadata::sim_id field.
    [[nodiscard]] fastscope::Result<std::vector<SignalMetadata>>
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
    [[nodiscard]] fastscope::Result<std::shared_ptr<SignalBuffer>>
    load_signal(const SignalMetadata& meta) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace fastscope

