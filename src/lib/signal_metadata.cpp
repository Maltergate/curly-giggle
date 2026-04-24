#include "fastscope/signal_metadata.hpp"

namespace fastscope {

const char* data_type_name(DataType dt) noexcept
{
    switch (dt) {
        case DataType::Float32:  return "float32";
        case DataType::Float64:  return "float64";
        case DataType::Int32:    return "int32";
        case DataType::Int64:    return "int64";
        case DataType::UInt32:   return "uint32";
        case DataType::UInt64:   return "uint64";
        case DataType::Unknown:  [[fallthrough]];
        default:                 return "unknown";
    }
}

} // namespace fastscope
