#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <source_location>
#include <system_error>

namespace gnc {

// ── Error codes ────────────────────────────────────────────────────────────────

enum class ErrorCode : int {
    // Generic
    Unknown         = 0,
    // I/O
    FileNotFound    = 100,
    FileOpenFailed  = 101,
    FileReadFailed  = 102,
    // HDF5
    HDF5OpenFailed  = 200,
    HDF5ReadFailed  = 201,
    HDF5TypeMismatch= 202,
    // Signal processing
    SignalNotFound  = 300,
    SignalTypeMismatch = 301,
    DimensionMismatch  = 302,
    // Rendering / UI
    RenderFailed    = 400,
};

std::string_view error_code_name(ErrorCode code) noexcept;

// ── Error struct ───────────────────────────────────────────────────────────────

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
//   gnc::Result<SignalBuffer>  load_signal(const SignalMetadata&);
//   gnc::Result<void>          write_file(const std::string& path);
//
// Propagation:
//   auto buf = GNC_TRY(load_signal(meta));   // returns early on error
//   auto val = result.value();               // throws on error (if RTTI enabled)
//   if (!result) { handle(result.error()); }
//
template<typename T>
using Result = std::expected<T, Error>;

// Convenience alias for operations with no return value.
using VoidResult = Result<void>;

// ── GNC_TRY macro ──────────────────────────────────────────────────────────────
//
// Early-return on error inside a function returning gnc::Result<U>.
//
//   gnc::Result<Foo> my_func() {
//       auto x = GNC_TRY(step_one());          // x is the unwrapped value
//       auto y = GNC_TRY(step_two(x));         // y is the unwrapped value
//       return Foo{x, y};
//   }
//
#define GNC_TRY(expr)                                        \
    ({                                                       \
        auto _gnc_res = (expr);                              \
        if (!_gnc_res) return std::unexpected(_gnc_res.error()); \
        std::move(*_gnc_res);                                \
    })

// ── Helper factories ───────────────────────────────────────────────────────────

template<typename T>
[[nodiscard]] inline Result<T> make_error(ErrorCode code,
                                          std::string message,
                                          std::string context = {})
{
    return std::unexpected(Error::make(code, std::move(message), std::move(context)));
}

[[nodiscard]] inline VoidResult make_void_error(ErrorCode code,
                                                std::string message,
                                                std::string context = {})
{
    return std::unexpected(Error::make(code, std::move(message), std::move(context)));
}

} // namespace gnc
