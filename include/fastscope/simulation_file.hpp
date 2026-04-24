#pragma once
/// @file simulation_file.hpp
/// @brief Owns one open HDF5 file and its signal cache.
/// @ingroup data_layer

#include "fastscope/error.hpp"
#include "fastscope/hdf5_reader.hpp"
#include "fastscope/signal_buffer.hpp"
#include "fastscope/signal_metadata.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace fastscope {

/// @brief Owns one open HDF5 file and its associated signal cache.
/// @details Wraps HDF5Reader with a unique sim_id, a user-chosen time axis,
///          and a weak-ptr buffer cache shared with PlottedSignal instances.
class SimulationFile {
public:
    explicit SimulationFile(std::string sim_id);
    ~SimulationFile() = default;

    SimulationFile(const SimulationFile&)            = delete;
    SimulationFile& operator=(const SimulationFile&) = delete;
    SimulationFile(SimulationFile&&)                  = default;
    SimulationFile& operator=(SimulationFile&&)       = default;

    /// Open the file and enumerate all datasets.
    /// On success, is_open() returns true and signals() is populated.
    fastscope::Result<void> open(const std::filesystem::path& path);

    /// Close the file and release all cached buffers.
    void close();

    [[nodiscard]] bool is_open() const noexcept;

    [[nodiscard]] const std::string&           sim_id()   const noexcept { return m_sim_id; }
    [[nodiscard]] const std::filesystem::path& path()     const noexcept { return m_path; }

    /// User-editable display name (defaults to the filename on open).
    [[nodiscard]] const std::string& display_name() const noexcept { return m_display_name; }
    void set_display_name(std::string name) { m_display_name = std::move(name); }

    /// The user-selected time axis H5 path.
    /// Empty string = synthesise 0-based sample index axis.
    [[nodiscard]] const std::string& time_axis() const noexcept { return m_time_axis; }
    void set_time_axis(std::string p) { m_time_axis = std::move(p); }

    /// All datasets enumerated from the file (includes candidate time axes).
    [[nodiscard]] const std::vector<SignalMetadata>& signals() const noexcept { return m_signals; }

    /// Heuristic time-axis candidates returned by suggest_time_axes().
    [[nodiscard]] const std::vector<std::string>& suggested_time_axes() const noexcept
    { return m_time_hints; }

    /// Load (or return from cache) the buffer for a signal.
    /// Uses the current time_axis() to set the time vector.
    fastscope::Result<std::shared_ptr<SignalBuffer>> load_signal(const SignalMetadata& meta);

    /// Release all cached weak_ptrs (does not free buffers held by callers).
    void evict_cache();

private:
    std::string                                        m_sim_id;
    std::string                                        m_display_name;
    std::filesystem::path                              m_path;
    HDF5Reader                                         m_reader;
    std::string                                        m_time_axis;
    std::vector<SignalMetadata>                        m_signals;
    std::vector<std::string>                           m_time_hints;
    std::map<std::string, std::weak_ptr<SignalBuffer>> m_cache;
};

} // namespace fastscope
