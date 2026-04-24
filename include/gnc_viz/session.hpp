#pragma once
/// @file session.hpp
/// @brief Session save/restore: serializes AppState to/from JSON.
/// @ingroup utilities

#include <filesystem>
#include <string>

namespace gnc_viz {

struct AppState;

/// @brief Default session file path: ~/.gnc_viz/session.json.
/// @return Filesystem path to the default session file.
[[nodiscard]] std::filesystem::path default_session_path();

/// @brief Save the current AppState to a session file.
/// @param state AppState to serialize.
/// @param path  Output JSON file path. Defaults to default_session_path().
/// @return false and logs a warning on failure (non-fatal). STUB until Phase 14.
bool save_session(const AppState& state,
                  const std::filesystem::path& path = default_session_path());

/// @brief Load AppState from a session file.
/// @param state Existing AppState to populate. Fields absent in the file keep their defaults.
/// @param path  Input JSON file path. Defaults to default_session_path().
/// @return false if the file doesn't exist or cannot be parsed. STUB until Phase 14.
bool load_session(AppState& state,
                  const std::filesystem::path& path = default_session_path());

} // namespace gnc_viz
