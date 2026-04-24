#pragma once
/// @file ground_track_plot.hpp
/// @brief IPlotType implementation for satellite ground track plots.
/// @ingroup plot_system
#include "fastscope/interfaces.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace fastscope {

class SignalBuffer;

/// @brief IPlotType implementation for satellite ground track plots.
/// @details Converts ECEF/ECI position signals to latitude/longitude and
///          renders them as a scatter plot on a cylindrical map.
class GroundTrackPlot : public IPlotType {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Ground Track"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "groundtrack"; }
    void render(AppState& state, float width, float height) override;
    void on_activate(AppState&) override {}
    void on_deactivate() override {}

private:
    /// @brief Cached pre-computed lat/lon arrays for one signal (keyed by plot_key).
    struct GroundTrackCache {
        std::vector<float>           lats;
        std::vector<float>           lons;
        std::weak_ptr<SignalBuffer>  source;
    };
    std::unordered_map<std::string, GroundTrackCache> m_cache;

    static void build_ground_track(GroundTrackCache& cache, const SignalBuffer& buf);
};

} // namespace fastscope
