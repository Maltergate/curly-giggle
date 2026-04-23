#pragma once

// ── AxisManager — multi-Y-axis state (max 3 axes, pure state, no ImPlot dep) ─
//
// ImPlot supports max 3 Y-axes (Y1=left, Y2=right, Y3=third).
// AxisManager tracks which plotted signal (by plot_key) is on which axis,
// and holds the AxisConfig for each axis.
// The rendering code (timeseries_plot.cpp) reads this state and translates
// it to ImPlot calls.

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace gnc_viz {

/// Configuration for one Y-axis (up to 3 in ImPlot).
struct AxisConfig {
    int         id          = 0;    ///< 0=Y1(left), 1=Y2(right), 2=Y3
    std::string label;              ///< e.g. "Position [km]"
    std::string units;              ///< e.g. "km"
    bool        auto_range  = true;
    double      range_min   = 0.0;
    double      range_max   = 1.0;
};

class AxisManager {
public:
    static constexpr int max_axes = 3;

    AxisManager();

    /// Assign a plotted signal (identified by plot_key) to an axis (0, 1, or 2).
    /// Creates the AxisConfig if it doesn't exist yet.
    /// Clamps axis_id to [0, max_axes-1].
    void assign(const std::string& plot_key, int axis_id);

    /// Release a signal's axis assignment (call when signal is removed from plot).
    void release(const std::string& plot_key);

    /// Returns the axis id for a given plot_key. Returns 0 (default) if not assigned.
    [[nodiscard]] int get_axis_for(const std::string& plot_key) const;

    /// Returns the configs for all axes that have at least one signal assigned.
    /// Always includes axis 0 (the primary Y-axis), even if empty.
    [[nodiscard]] std::vector<AxisConfig> active_axes() const;

    /// Get or create an AxisConfig for the given axis id.
    AxisConfig& axis_config(int axis_id);

    /// Returns true if any signal is assigned to axis_id.
    [[nodiscard]] bool has_signals_on(int axis_id) const;

    /// Total number of distinct axis ids with at least one signal.
    [[nodiscard]] int active_axis_count() const;

    /// Release all assignments and reset configs (e.g. when all signals removed).
    void clear();

    /// Release only the signal-to-axis assignments without touching AxisConfig
    /// (labels, ranges, auto_range). Use this instead of clear() when you want
    /// to re-sync assignments every frame without losing user-configured axis state.
    void clear_assignments();

private:
    std::unordered_map<std::string, int> m_assignments; // plot_key → axis_id
    std::array<AxisConfig, max_axes>     m_configs;     // indexed by axis_id
};

} // namespace gnc_viz
