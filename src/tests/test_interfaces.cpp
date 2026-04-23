#include <catch2/catch_test_macros.hpp>
#include "gnc_viz/interfaces.hpp"
#include "gnc_viz/app_state.hpp"

using namespace gnc_viz;

// ── Minimal concrete implementations for testing ──────────────────────────────

struct MockPlotType : IPlotType {
    std::string_view name() const noexcept override { return "Mock Plot"; }
    std::string_view id()   const noexcept override { return "mock"; }
    void render(const AppState&, float, float) override { rendered = true; }
    bool rendered = false;
};

struct MockSignalOp : ISignalOperation {
    std::string_view name()        const noexcept override { return "Mock Op"; }
    std::string_view id()          const noexcept override { return "mock_op"; }
    int              input_count() const noexcept override { return 2; }
    gnc::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>>) override {
        called = true;
        return gnc::make_error<std::shared_ptr<SignalBuffer>>(
            gnc::ErrorCode::Unknown, "not implemented in mock");
    }
    bool called = false;
};

struct MockTool : IVisualizationTool {
    std::string_view name()  const noexcept override { return "Mock Tool"; }
    std::string_view id()    const noexcept override { return "mock_tool"; }
    std::string_view icon()  const noexcept override { return "T"; }
    void handle_input(AppState&) override  { input_called = true; }
    void render_overlay(const AppState&) override { overlay_called = true; }
    bool input_called   = false;
    bool overlay_called = false;
};

// ── IPlotType concept checks ───────────────────────────────────────────────────

TEST_CASE("MockPlotType satisfies PlotTypeConcept", "[interfaces]")
{
    static_assert(PlotTypeConcept<MockPlotType>,
                  "MockPlotType must satisfy PlotTypeConcept");
    SUCCEED("Concept check passed at compile time");
}

TEST_CASE("IPlotType virtual methods dispatch correctly", "[interfaces]")
{
    MockPlotType p;
    REQUIRE(p.name() == "Mock Plot");
    REQUIRE(p.id()   == "mock");
    REQUIRE(p.rendered == false);

    AppState s;
    p.render(s, 800.0f, 600.0f);
    REQUIRE(p.rendered == true);
}

// ── ISignalOperation concept checks ───────────────────────────────────────────

TEST_CASE("MockSignalOp satisfies SignalOperationConcept", "[interfaces]")
{
    static_assert(SignalOperationConcept<MockSignalOp>,
                  "MockSignalOp must satisfy SignalOperationConcept");
    SUCCEED("Concept check passed at compile time");
}

TEST_CASE("ISignalOperation virtual methods dispatch correctly", "[interfaces]")
{
    MockSignalOp op;
    REQUIRE(op.name()        == "Mock Op");
    REQUIRE(op.id()          == "mock_op");
    REQUIRE(op.input_count() == 2);
    REQUIRE(op.called        == false);

    std::vector<std::shared_ptr<SignalBuffer>> inputs;
    auto result = op.execute(inputs);
    REQUIRE(op.called == true);
    REQUIRE(!result.has_value());
    REQUIRE(result.error().code == gnc::ErrorCode::Unknown);
}

// ── IVisualizationTool concept checks ─────────────────────────────────────────

TEST_CASE("MockTool satisfies VisualizationToolConcept", "[interfaces]")
{
    static_assert(VisualizationToolConcept<MockTool>,
                  "MockTool must satisfy VisualizationToolConcept");
    SUCCEED("Concept check passed at compile time");
}

TEST_CASE("IVisualizationTool virtual methods dispatch correctly", "[interfaces]")
{
    MockTool t;
    REQUIRE(t.name() == "Mock Tool");
    REQUIRE(t.id()   == "mock_tool");
    REQUIRE(t.icon() == "T");

    AppState s;
    t.handle_input(s);
    t.render_overlay(s);
    REQUIRE(t.input_called   == true);
    REQUIRE(t.overlay_called == true);
}
