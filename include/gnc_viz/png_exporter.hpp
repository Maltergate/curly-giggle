#pragma once
/// @file png_exporter.hpp
/// @brief Exports the current plot view as a PNG screenshot.
/// @ingroup utilities
#include "gnc_viz/error.hpp"
#include <filesystem>
#include <string>

namespace gnc_viz {

/// @brief Export the current screen as a PNG screenshot using macOS screencapture.
/// @details Invokes screencapture in interactive mode so the user can select a region.
///          Must be called from the main thread.
/// @param window_title  Currently unused (reserved for future window-targeted capture).
/// @param path          Output file path (.png).
/// @return Ok(true) on success, error on failure.
gnc::Result<bool> export_png(const std::string& window_title,
                              const std::filesystem::path& path);

} // namespace gnc_viz
