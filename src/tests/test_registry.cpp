#include <catch2/catch_test_macros.hpp>
#include "gnc_viz/registry.hpp"
#include "gnc_viz/interfaces.hpp"
#include "gnc_viz/app_state.hpp"

using namespace gnc_viz;

// ── Test doubles ──────────────────────────────────────────────────────────────

struct AlphaPlot : IPlotType {
    std::string_view name() const noexcept override { return "Alpha"; }
    std::string_view id()   const noexcept override { return "alpha"; }
    void render(AppState&, float, float) override {}
};

struct BetaPlot : IPlotType {
    std::string_view name() const noexcept override { return "Beta"; }
    std::string_view id()   const noexcept override { return "beta"; }
    void render(AppState&, float, float) override {}
};

struct AlphaOp : ISignalOperation {
    std::string_view name()        const noexcept override { return "AlphaOp"; }
    std::string_view id()          const noexcept override { return "alpha_op"; }
    int              input_count() const noexcept override { return 1; }
    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>>) override {
        return gnc::make_error<std::shared_ptr<SignalBuffer>>(
            gnc::ErrorCode::Unknown, "stub");
    }
};

// ── Registry<IPlotType> tests ─────────────────────────────────────────────────

TEST_CASE("Registry is empty on construction", "[registry]")
{
    PlotRegistry reg;
    REQUIRE(reg.empty());
    REQUIRE(reg.size() == 0u);
}

TEST_CASE("Registry::register_type adds an entry", "[registry]")
{
    PlotRegistry reg;
    reg.register_type<AlphaPlot>("alpha");
    REQUIRE(!reg.empty());
    REQUIRE(reg.size() == 1u);
    REQUIRE(reg.contains("alpha"));
}

TEST_CASE("Registry::create returns correct concrete type", "[registry]")
{
    PlotRegistry reg;
    reg.register_type<AlphaPlot>("alpha");
    reg.register_type<BetaPlot>("beta");

    auto a = reg.create("alpha");
    REQUIRE(a != nullptr);
    REQUIRE(a->id() == "alpha");

    auto b = reg.create("beta");
    REQUIRE(b != nullptr);
    REQUIRE(b->id() == "beta");
}

TEST_CASE("Registry::create returns nullptr for unknown key", "[registry]")
{
    PlotRegistry reg;
    REQUIRE(reg.create("no_such_key") == nullptr);
}

TEST_CASE("Registry::keys preserves insertion order", "[registry]")
{
    PlotRegistry reg;
    reg.register_type<AlphaPlot>("alpha");
    reg.register_type<BetaPlot>("beta");
    REQUIRE(reg.keys().size() == 2u);
    REQUIRE(reg.keys()[0] == "alpha");
    REQUIRE(reg.keys()[1] == "beta");
}

TEST_CASE("Registry::register_type overwrites existing key", "[registry]")
{
    PlotRegistry reg;
    reg.register_type<AlphaPlot>("myplot");
    reg.register_type<BetaPlot>("myplot");   // overwrite
    REQUIRE(reg.size() == 1u);
    auto p = reg.create("myplot");
    REQUIRE(p->id() == "beta");
}

TEST_CASE("Registry::register_factory allows custom factory", "[registry]")
{
    PlotRegistry reg;
    int factory_call_count = 0;
    reg.register_factory("custom", [&]() -> std::unique_ptr<IPlotType> {
        ++factory_call_count;
        return std::make_unique<AlphaPlot>();
    });
    auto p = reg.create("custom");
    REQUIRE(p != nullptr);
    REQUIRE(factory_call_count == 1);
}

TEST_CASE("OperationRegistry works with ISignalOperation", "[registry]")
{
    OperationRegistry reg;
    reg.register_type<AlphaOp>("alpha_op");
    REQUIRE(reg.contains("alpha_op"));
    auto op = reg.create("alpha_op");
    REQUIRE(op != nullptr);
    REQUIRE(op->input_count() == 1);
}
