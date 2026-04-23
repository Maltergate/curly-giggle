#pragma once

#include <string_view>

namespace gnc_viz {

// Compile-time version constants
inline constexpr int    VERSION_MAJOR = 0;
inline constexpr int    VERSION_MINOR = 1;
inline constexpr int    VERSION_PATCH = 0;
inline constexpr std::string_view VERSION_STRING = "0.1.0";

/// Runtime accessor — use VERSION_STRING when possible.
const char* version() noexcept;

} // namespace gnc_viz
