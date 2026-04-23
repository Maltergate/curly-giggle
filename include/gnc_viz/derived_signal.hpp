#pragma once

#include "gnc_viz/signal_buffer.hpp"
#include "gnc_viz/interfaces.hpp"
#include "gnc_viz/error.hpp"
#include "gnc_viz/registry.hpp"

#include <memory>
#include <string>
#include <vector>

namespace gnc_viz {

/// DerivedSignal: computes a new SignalBuffer from N input buffers + an ISignalOperation.
/// The result is cached after first computation; call invalidate() to force recomputation.
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
    [[nodiscard]] gnc::Result<std::shared_ptr<SignalBuffer>> compute();

    /// Force recomputation on next compute() call.
    void invalidate() noexcept;

    /// True if a cached result is available (compute() has been called).
    [[nodiscard]] bool is_computed() const noexcept;

private:
    std::shared_ptr<SignalBuffer> m_cached;
    bool m_computed = false;
};

} // namespace gnc_viz
