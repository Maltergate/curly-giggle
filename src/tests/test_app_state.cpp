#include <catch2/catch_test_macros.hpp>
#include "gnc_viz/app_state.hpp"

using gnc_viz::AppState;

TEST_CASE("AppState default-constructs", "[app_state]")
{
    AppState s;
    REQUIRE(s.quit_requested == false);
    REQUIRE(s.panes.file_pane_visible   == true);
    REQUIRE(s.panes.signal_pane_visible == true);
    REQUIRE(s.panes.file_pane_width     == 280.0f);
    REQUIRE(s.panes.signal_pane_width   == 300.0f);
}

TEST_CASE("PaneState can be mutated", "[app_state]")
{
    AppState s;
    s.panes.file_pane_visible = false;
    REQUIRE(s.panes.file_pane_visible == false);
    s.panes.file_pane_width = 150.0f;
    REQUIRE(s.panes.file_pane_width == 150.0f);
}

TEST_CASE("DebugState defaults to all false", "[app_state]")
{
    AppState s;
    REQUIRE(s.debug.show_imgui_demo  == false);
    REQUIRE(s.debug.show_implot_demo == false);
    REQUIRE(s.debug.show_metrics     == false);
}

TEST_CASE("quit_requested can be set", "[app_state]")
{
    AppState s;
    s.quit_requested = true;
    REQUIRE(s.quit_requested == true);
}

TEST_CASE("AppState holds simulations and plotted_signals", "[app_state]")
{
    AppState s;
    REQUIRE(s.simulations.empty());
    REQUIRE(s.plotted_signals.empty());
}
