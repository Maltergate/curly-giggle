#pragma once
/// @file derived_signal.hpp
/// @brief Lazily-computed derived signal from an ISignalOperation and input buffers.
/// @ingroup signal_ops

#include "fastscope/signal_buffer.hpp"
#include "fastscope/interfaces.hpp"
#include "fastscope/error.hpp"
#include "fastscope/registry.hpp"

#include <memory>
#include <string>
#include <vector>

namespace fastscope {

/// @brief Lazily-computed derived signal from an ISignalOperation and input buffers.
/// @details The result is cached after first computation; call invalidate() to force recomputation.
///          Identified by a unique slug id and a human-readable display_name.
struct DerivedSignal {
    /// Unique identifier (slug format, e.g. "derived_0")
    std::string id;

    /// Human-readable display name (e.g. "‖r_eci‖")
    std::string display_name;

    /// The operation to apply (owned)
    std::shared_ptr<ISignalOperation> operation;

    /// The input buffers (in order, matching operation::input_count())
    std::vector<std::shared_ptr<SignalBuffer>> inputs;

    // ── Lazy computation ──────────────────────────────────────────────────────

    /// Compute (or return cached) result buffer.
    /// Returns error if:
    ///   - operation is null
    ///   - wrong number of inputs
    ///   - operation::execute() fails
    [[nodiscard]] fastscope::Result<std::shared_ptr<SignalBuffer>> compute();

    /// Force recomputation on next compute() call.
    void invalidate() noexcept;

    /// True if a cached result is available (compute() has been called).
    [[nodiscard]] bool is_computed() const noexcept;

private:
    std::shared_ptr<SignalBuffer> m_cached;
    bool m_computed = false;
};

} // namespace fastscope
