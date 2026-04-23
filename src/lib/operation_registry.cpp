#include "gnc_viz/operation_registry.hpp"
#include "gnc_viz/signal_ops.hpp"

namespace gnc_viz {

OperationRegistry create_operation_registry()
{
    OperationRegistry reg;
    reg.register_type<AddOp>("add");
    reg.register_type<SubtractOp>("subtract");
    reg.register_type<MultiplyOp>("multiply");
    reg.register_type<ScaleOp>("scale");
    reg.register_type<MagnitudeOp>("magnitude");
    return reg;
}

} // namespace gnc_viz
