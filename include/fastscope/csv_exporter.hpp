#pragma once
/// @file csv_exporter.hpp
/// @brief Exports visible plotted signals to a CSV file.
/// @ingroup utilities
#include "fastscope/app_state.hpp"
#include "fastscope/error.hpp"
#include <filesystem>

namespace fastscope {

/// Export all visible plotted signals as a CSV file.
/// Columns: time, signal1, signal2, ... (one column per component per signal).
/// If signals have different time vectors, the time vector of the first visible
/// signal is used as the master grid; other signals are matched by nearest index
/// via std::lower_bound (no interpolation).
///
/// @param state   AppState with plotted_signals (buffers must be loaded)
/// @param path    Output file path
/// @returns       Ok(number_of_rows_written) or error
fastscope::Result<std::size_t> export_csv(const AppState& state,
                                     const std::filesystem::path& path);

} // namespace fastscope
