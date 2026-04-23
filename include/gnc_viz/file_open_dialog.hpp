#pragma once

// ── FileOpenDialog — native macOS file picker ─────────────────────────────────
//
// Thin pure-C++ wrapper around NSOpenPanel (Obj-C++ impl in file_open_dialog.mm).
// Presents a modal sheet; returns after the user confirms or cancels.
//
// Usage:
//   auto result = gnc_viz::show_open_dialog();
//   if (result.confirmed) {
//       for (const auto& path : result.paths)
//           load_simulation(path);
//   }

#include <filesystem>
#include <vector>

namespace gnc_viz {

struct OpenDialogResult {
    bool confirmed = false;
    std::vector<std::filesystem::path> paths;
};

/// Show a modal "Open" dialog.
/// @param title            Title shown in the panel.
/// @param allow_multiple   If true, the user may select multiple files.
/// @param allowed_types    File-type extensions to filter by (e.g. {"h5", "hdf5"}).
///                         Pass an empty vector to allow all types.
OpenDialogResult show_open_dialog(
    const char*              title          = "Open Simulation File",
    bool                     allow_multiple = true,
    std::vector<std::string> allowed_types  = {"h5", "hdf5"});

} // namespace gnc_viz
