#pragma once
/// @file plotted_signal.hpp
/// @brief Represents one signal the user has added to the active plot.
/// @ingroup app_layer

#include "fastscope/signal_buffer.hpp"
#include "fastscope/signal_metadata.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace fastscope {

/// @brief Represents one signal the user has added to the active plot.
/// @details Carries identity (sim_id + metadata), a lazy-loaded buffer,
///          display properties (color, alias, visibility, y_axis), and
///          pre-extracted float caches for zero-allocation ImPlot rendering.
struct PlottedSignal {
    // ── Identity ──────────────────────────────────────────────────────────────
    std::string    sim_id;  ///< Which SimulationFile this signal comes from.
    SignalMetadata meta;    ///< Cached metadata (h5_path, name, shape, units, …).

    // ── Data ──────────────────────────────────────────────────────────────────
    /// Loaded buffer. Null until first render. Shared with SimulationFile cache.
    std::shared_ptr<SignalBuffer> buffer;

    // ── Display ───────────────────────────────────────────────────────────────
    uint32_t    color_rgba      = 0xFFFFFFFF; ///< Packed 0xRRGGBBAA from ColorManager.
    std::string alias;                        ///< User-editable label (default: meta.name).
    bool        visible         = true;
    int         y_axis          = 0;          ///< 0=left, 1=right, 2=third Y axis.

    /// For vector signals: -1 = plot all components; ≥0 = single component.
    int component_index = -1;

    // ── Per-frame render cache ────────────────────────────────────────────────
    /// Pre-extracted per-component arrays (row-major → columnar). Populated once
    /// after buffer loads; avoids per-frame heap allocation in the render loop.
    /// time_cache_f[i] = time of sample i, converted to float once at load time.
    /// Passed directly to ImPlot (which renders in float internally).
    std::vector<float> time_cache_f;

    /// component_cache[k][i] = sample i of component k, converted to float once
    /// at load time. Scalars use component_cache[0]. Sized K×N after build.
    std::vector<std::vector<float>> component_cache;

    /// Points at the buffer that populated the float caches. If the buffer pointer
    /// changes (e.g. time-axis switch) the caches are stale and must be rebuilt.
    std::weak_ptr<SignalBuffer> cache_source;

    /// Cached legend label "[SimName] signal_name". Built once when the signal
    /// is first rendered and stored here to avoid per-frame linear search +
    /// heap allocation. Invalidated (cleared) if sim display name or alias changes.
    std::string cached_label;

    /// Cached plot key "<sim_id>:<h5_path>". Built once on first use to avoid
    /// per-frame string concatenation in axis synchronization loop.
    std::string cached_plot_key;

    // ── Helpers ───────────────────────────────────────────────────────────────

    /// Unique key for ImPlot series ID and ColorManager: "<sim_id>:<h5_path>".
    /// Returns cached version if available; builds and caches on first call.
    [[nodiscard]] const std::string& plot_key_cached() const {
        // const_cast needed because we cache the result as a performance optimization
        auto& self = const_cast<PlottedSignal&>(*this);
        if (self.cached_plot_key.empty() && (!sim_id.empty() || !meta.h5_path.empty())) {
            self.cached_plot_key = sim_id + ":" + meta.h5_path;
        }
        return cached_plot_key;
    }

    /// Unique key for ImPlot series ID and ColorManager: "<sim_id>:<h5_path>".
    [[nodiscard]] std::string plot_key() const { return sim_id + ":" + meta.h5_path; }

    /// Display label for the legend: alias if set, otherwise full HDF5 path.
    [[nodiscard]] const std::string& display_name() const noexcept
    {
        return alias.empty() ? meta.h5_path : alias;
    }
};

} // namespace fastscope
