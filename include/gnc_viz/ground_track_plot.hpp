#pragma once
#include "gnc_viz/interfaces.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace gnc_viz {

class SignalBuffer;

class GroundTrackPlot : public IPlotType {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Ground Track"; }
    [[nodiscard]] std::string_view id()   const noexcept override { return "groundtrack"; }
    void render(AppState& state, float width, float height) override;
    void on_activate(AppState&) override {}
    void on_deactivate() override {}

private:
    // Pre-computed lat/lon per signal (plot_key → cache). Valid while buffer ptr unchanged.
    struct GroundTrackCache {
        std::vector<float>           lats;
        std::vector<float>           lons;
        std::weak_ptr<SignalBuffer>  source;
    };
    std::unordered_map<std::string, GroundTrackCache> m_cache;

    static void build_ground_track(GroundTrackCache& cache, const SignalBuffer& buf);
};

} // namespace gnc_viz
