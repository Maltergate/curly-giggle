#include "gnc_viz/error.hpp"
#include <format>

namespace gnc {

std::string_view error_code_name(ErrorCode code) noexcept
{
    switch (code) {
        case ErrorCode::Unknown:           return "Unknown";
        case ErrorCode::NotFound:          return "NotFound";
        case ErrorCode::InvalidArgument:   return "InvalidArgument";
        case ErrorCode::IOError:           return "IOError";
        case ErrorCode::InvalidState:      return "InvalidState";
        case ErrorCode::FileNotFound:      return "FileNotFound";
        case ErrorCode::FileOpenFailed:    return "FileOpenFailed";
        case ErrorCode::FileReadFailed:    return "FileReadFailed";
        case ErrorCode::HDF5OpenFailed:    return "HDF5OpenFailed";
        case ErrorCode::HDF5ReadFailed:    return "HDF5ReadFailed";
        case ErrorCode::HDF5TypeMismatch:  return "HDF5TypeMismatch";
        case ErrorCode::SignalNotFound:    return "SignalNotFound";
        case ErrorCode::SignalTypeMismatch:return "SignalTypeMismatch";
        case ErrorCode::DimensionMismatch: return "DimensionMismatch";
        case ErrorCode::RenderFailed:      return "RenderFailed";
    }
    return "Unknown";
}

std::string Error::to_string() const
{
    if (context.empty())
        return std::format("{}: {}", error_code_name(code), message);
    return std::format("{}: {} [{}]", error_code_name(code), message, context);
}

} // namespace gnc
