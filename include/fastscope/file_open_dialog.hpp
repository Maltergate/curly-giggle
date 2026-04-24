#pragma once
/// @file file_open_dialog.hpp
/// @brief Native macOS file open/save dialog wrappers.
/// @ingroup utilities

#include <filesystem>
#include <vector>

namespace fastscope {

/// @brief Result of a native open-file dialog.
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

// ── Save dialog ────────────────────────────────────────────────────────────────

/// @brief Result of a native save-file dialog.
struct SaveDialogResult {
    bool                  confirmed = false;
    std::filesystem::path path;
};

/// Show a modal "Save" dialog backed by NSSavePanel.
/// @param title        Dialog window title.
/// @param extensions   Allowed file extensions, e.g. {"csv"} or {"png"}.
/// @param default_name Default filename shown in the name field (no extension).
SaveDialogResult show_save_dialog(
    const std::string&              title,
    const std::vector<std::string>& extensions,
    const std::string&              default_name = "export");

} // namespace fastscope
