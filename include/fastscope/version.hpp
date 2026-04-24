#pragma once
/// @file version.hpp
/// @brief Compile-time and runtime version constants for FastScope.
/// @ingroup utilities

#include <string_view>

namespace fastscope {

/// @brief Major version number.
inline constexpr int    VERSION_MAJOR = 0;
/// @brief Minor version number.
inline constexpr int    VERSION_MINOR = 1;
/// @brief Patch version number.
inline constexpr int    VERSION_PATCH = 0;
/// @brief Full version string (e.g. "0.1.0").
inline constexpr std::string_view VERSION_STRING = "0.1.0";

/// @brief Runtime version accessor.
/// @return Null-terminated version string (same value as VERSION_STRING).
const char* version() noexcept;

} // namespace fastscope
