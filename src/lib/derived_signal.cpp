#include "fastscope/derived_signal.hpp"
#include "fastscope/error.hpp"

#include <span>

namespace fastscope {

fastscope::Result<std::shared_ptr<SignalBuffer>> DerivedSignal::compute()
{
    if (m_computed)
        return fastscope::Result<std::shared_ptr<SignalBuffer>>{m_cached};

    if (!operation)
        return fastscope::make_error<std::shared_ptr<SignalBuffer>>(
            fastscope::ErrorCode::InvalidState, "DerivedSignal: operation is null");

    const int required = operation->input_count();
    const auto n = static_cast<int>(inputs.size());
    if (required == -1) {
        if (n < 2)
            return fastscope::make_error<std::shared_ptr<SignalBuffer>>(
                fastscope::ErrorCode::InvalidArgument,
                "DerivedSignal: variadic operation requires at least 2 inputs");
    } else {
        if (n != required)
            return fastscope::make_error<std::shared_ptr<SignalBuffer>>(
                fastscope::ErrorCode::InvalidArgument,
                "DerivedSignal: wrong number of inputs for operation");
    }

    auto result = operation->execute(std::span<const std::shared_ptr<SignalBuffer>>{inputs});
    if (!result)
        return std::unexpected(result.error());

    m_cached   = *result;
    m_computed = true;
    return fastscope::Result<std::shared_ptr<SignalBuffer>>{m_cached};
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

} // namespace fastscope
