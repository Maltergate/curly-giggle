#pragma once
/// @file operation_registry.hpp
/// @brief Factory function that creates a pre-populated OperationRegistry.
/// @ingroup signal_ops
#include "fastscope/registry.hpp"

namespace fastscope {
    /// @brief Create and return an OperationRegistry pre-populated with all built-in operations.
    /// @return OperationRegistry containing AddOp, SubtractOp, MultiplyOp, ScaleOp, MagnitudeOp.
    OperationRegistry create_operation_registry();
} // namespace fastscope
