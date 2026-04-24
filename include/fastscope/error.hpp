#pragma once
/// @file error.hpp
/// @brief Error types and Result<T> alias for FastScope error handling.
/// @ingroup core_interfaces

#include <expected>
#include <string>
#include <string_view>
#include <source_location>
#include <system_error>

namespace fastscope {

/// @brief Error codes for all FastScope failure categories.
// ── Error codes ────────────────────────────────────────────────────────────────

enum class ErrorCode : int {
    // Generic
    /// @brief Unknown or unclassified error.
    Unknown         = 0,
    /// @brief Requested item was not found.
    NotFound        = 1,
    /// @brief A function argument is invalid.
    InvalidArgument = 2,
    /// @brief Generic I/O error.
    IOError         = 3,
    /// @brief Operation is invalid in the current state.
    InvalidState    = 4,
    // I/O
    /// @brief The specified file was not found.
    FileNotFound    = 100,
    /// @brief The file could not be opened.
    FileOpenFailed  = 101,
    /// @brief The file could not be read.
    FileReadFailed  = 102,
    // HDF5
    /// @brief An HDF5 file could not be opened.
    HDF5OpenFailed  = 200,
    /// @brief An HDF5 dataset could not be read.
    HDF5ReadFailed  = 201,
    /// @brief HDF5 dataset type does not match expected type.
    HDF5TypeMismatch= 202,
    // Signal processing
    /// @brief The requested signal was not found.
    SignalNotFound  = 300,
    /// @brief Signal types are incompatible.
    SignalTypeMismatch = 301,
    /// @brief Array dimensions are incompatible.
    DimensionMismatch  = 302,
    // Rendering / UI
    /// @brief A rendering operation failed.
    RenderFailed    = 400,
};

/// @brief Returns a human-readable name for the given ErrorCode.
/// @param code The error code to name.
/// @return A string_view of the error code name.
std::string_view error_code_name(ErrorCode code) noexcept;

// ── Error struct ───────────────────────────────────────────────────────────────

/// @brief Carries a structured error with code, message, and optional context.
struct Error {
    ErrorCode   code    = ErrorCode::Unknown;
    std::string message;
    std::string context;   // optional: calling site / filename / path

    /// Convenience constructor — most callers only need code + message.
    static Error make(ErrorCode code,
                      std::string message,
                      std::string context = {}) noexcept
    {
        return Error{code, std::move(message), std::move(context)};
    }

    /// Human-readable summary, e.g. "FileNotFound: /path/to/file.h5 [context]"
    [[nodiscard]] std::string to_string() const;
};

// ── Result<T> alias ────────────────────────────────────────────────────────────
//
// Usage:
//   fastscope::Result<SignalBuffer>  load_signal(const SignalMetadata&);
//   fastscope::Result<void>          write_file(const std::string& path);
//
// Propagation:
//   auto buf = FASTSCOPE_TRY(load_signal(meta));   // returns early on error
//   auto val = result.value();               // throws on error (if RTTI enabled)
//   if (!result) { handle(result.error()); }
//
template<typename T>
using Result = std::expected<T, Error>;

// Convenience alias for operations with no return value.
using VoidResult = Result<void>;

/// @brief Early-return propagation macro for fastscope::Result<T>.
/// @details Evaluates expr; if it holds an error, returns it immediately.
///          Otherwise, unwraps and yields the contained value.
#define FASTSCOPE_TRY(expr)                                        \
    ({                                                       \
        auto _gnc_res = (expr);                              \
        if (!_gnc_res) return std::unexpected(_gnc_res.error()); \
        std::move(*_gnc_res);                                \
    })

// ── Helper factories ───────────────────────────────────────────────────────────

/// @brief Create a Result<T> holding an error.
/// @tparam T The success value type.
/// @param code    Error code.
/// @param message Human-readable message.
/// @param context Optional calling-site context string.
/// @return An unexpected Result carrying the constructed Error.
template<typename T>
[[nodiscard]] inline Result<T> make_error(ErrorCode code,
                                          std::string message,
                                          std::string context = {})
{
    return std::unexpected(Error::make(code, std::move(message), std::move(context)));
}

/// @brief Create a VoidResult holding an error.
/// @param code    Error code.
/// @param message Human-readable message.
/// @param context Optional calling-site context string.
/// @return An unexpected VoidResult carrying the constructed Error.
[[nodiscard]] inline VoidResult make_void_error(ErrorCode code,
                                                std::string message,
                                                std::string context = {})
{
    return std::unexpected(Error::make(code, std::move(message), std::move(context)));
}

} // namespace fastscope
