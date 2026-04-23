#pragma once
#include "gnc_viz/app_state.hpp"
#include "gnc_viz/error.hpp"
#include <filesystem>

namespace gnc_viz {

/// Export all visible plotted signals as a CSV file.
/// Columns: time, signal1, signal2, ... (one column per component per signal).
/// If signals have different time vectors, the time vector of the first visible
/// signal is used as the master grid; other signals are matched by nearest index
/// via std::lower_bound (no interpolation).
///
/// @param state   AppState with plotted_signals (buffers must be loaded)
/// @param path    Output file path
/// @returns       Ok(number_of_rows_written) or error
gnc::Result<std::size_t> export_csv(const AppState& state,
                                     const std::filesystem::path& path);

} // namespace gnc_viz
