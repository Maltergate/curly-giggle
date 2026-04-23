#include "gnc_viz/derived_signal.hpp"
#include "gnc_viz/error.hpp"

#include <span>

namespace gnc_viz {

gnc::Result<std::shared_ptr<SignalBuffer>> DerivedSignal::compute()
{
    if (m_computed)
        return gnc::Result<std::shared_ptr<SignalBuffer>>{m_cached};

    if (!operation)
        return gnc::make_error<std::shared_ptr<SignalBuffer>>(
            gnc::ErrorCode::InvalidState, "DerivedSignal: operation is null");

    const int required = operation->input_count();
    const auto n = static_cast<int>(inputs.size());
    if (required == -1) {
        if (n < 2)
            return gnc::make_error<std::shared_ptr<SignalBuffer>>(
                gnc::ErrorCode::InvalidArgument,
                "DerivedSignal: variadic operation requires at least 2 inputs");
    } else {
        if (n != required)
            return gnc::make_error<std::shared_ptr<SignalBuffer>>(
                gnc::ErrorCode::InvalidArgument,
                "DerivedSignal: wrong number of inputs for operation");
    }

    auto result = operation->execute(std::span<const std::shared_ptr<SignalBuffer>>{inputs});
    if (!result)
        return std::unexpected(result.error());

    m_cached   = *result;
    m_computed = true;
    return gnc::Result<std::shared_ptr<SignalBuffer>>{m_cached};
}

void DerivedSignal::invalidate() noexcept
{
    m_computed = false;
    m_cached.reset();
}

bool DerivedSignal::is_computed() const noexcept
{
    return m_computed;
}

} // namespace gnc_viz
