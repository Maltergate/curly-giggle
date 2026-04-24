// test_system.cpp — integration-style system tests
//
// These tests exercise real end-to-end workflows using only fastscope_lib types.
// No HDF5 files, ImGui context, or PlotEngine required.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "fastscope/app_state.hpp"
#include "fastscope/axis_manager.hpp"
#include "fastscope/color_manager.hpp"
#include "fastscope/plotted_signal.hpp"
#include "fastscope/session.hpp"
#include "fastscope/signal_metadata.hpp"
#include "fastscope/simulation_file.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace fastscope;

// Shared temp path for session round-trip tests
static const fs::path k_sys_tmp =
    fs::temp_directory_path() / "fastscope_test_system.json";

static void remove_sys_tmp()
{
    std::error_code ec;
    fs::remove(k_sys_tmp, ec);
}

// ── 1. AppState default construction ─────────────────────────────────────────
//
// Verifies that AppState holds no simulations or plotted signals by default and
// that all lifecycle flags are in their expected initial state.

TEST_CASE("System: AppState default construction has empty data and correct flags",
          "[system][app_state]")
{
    AppState state;

    REQUIRE(state.simulations.empty());
    REQUIRE(state.plotted_signals.empty());
    REQUIRE(state.quit_requested == false);
    REQUIRE(state.panes.file_pane_visible   == true);
    REQUIRE(state.panes.signal_pane_visible == true);
    REQUIRE(state.axis_manager.active_axis_count() == 0);
    REQUIRE(state.colors.assigned_count() == 0);
}

// ── 2. SimulationFile: open nonexistent path returns error ───────────────────
//
// Verifies that opening a missing HDF5 file returns a fastscope::Result error and
// leaves the SimulationFile in a closed state while preserving the sim_id.

TEST_CASE("System: SimulationFile open nonexistent path returns error and stays closed",
          "[system][simulation_file]")
{
    SimulationFile sim("sim_test_42");
    REQUIRE(sim.sim_id() == "sim_test_42");
    REQUIRE(!sim.is_open());

    auto res = sim.open("/nonexistent/path/that/does/not/exist.h5");
    REQUIRE(!res);           // error result
    REQUIRE(!sim.is_open()); // still closed
    REQUIRE(sim.sim_id() == "sim_test_42"); // id unchanged
    REQUIRE(sim.signals().empty());         // no signals enumerated
}

// ── 3. AxisManager: clear_assignments preserves user-configured axis configs ─
//
// The render loop must NOT lose user-configured axis labels / ranges when it
// re-syncs assignments every frame.  clear_assignments() is the safe path.

TEST_CASE("System: AxisManager clear_assignments preserves configs, clears assignments",
          "[system][axis_manager]")
{
    AxisManager am;

    // Assign signals and configure axis 1
    am.assign("sim0:/attitude/q", 0);
    am.assign("sim0:/position/r", 1);
    am.axis_config(1).label      = "Position [km]";
    am.axis_config(1).auto_range = false;
    am.axis_config(1).range_min  = -7000.0;
    am.axis_config(1).range_max  =  7000.0;

    REQUIRE(am.has_signals_on(0));
    REQUIRE(am.has_signals_on(1));

    // clear_assignments() must remove signal assignments but preserve config
    am.clear_assignments();

    REQUIRE(!am.has_signals_on(0));
    REQUIRE(!am.has_signals_on(1));
    REQUIRE(am.active_axis_count() == 0);

    // Config must still be intact
    const AxisConfig& cfg = am.axis_config(1);
    REQUIRE(cfg.label      == "Position [km]");
    REQUIRE(cfg.auto_range == false);
    REQUIRE_THAT(cfg.range_min, Catch::Matchers::WithinAbs(-7000.0, 1e-9));
    REQUIRE_THAT(cfg.range_max, Catch::Matchers::WithinAbs( 7000.0, 1e-9));
}

// ── 4. Session round-trip: panes + axis config label ─────────────────────────
//
// Verifies a complex save→load cycle preserves pane dimensions, visibility
// flags, and user-configured axis labels.

TEST_CASE("System: Session round-trip preserves pane state and axis label",
          "[system][session]")
{
    remove_sys_tmp();

    // Build a custom state
    {
        AppState src;
        src.panes.file_pane_visible   = false;
        src.panes.signal_pane_visible = true;
        src.panes.file_pane_width     = 350.0f;
        src.panes.signal_pane_width   = 420.0f;
        src.axis_manager.axis_config(0).label = "Quaternion [-]";
        src.axis_manager.axis_config(0).auto_range = false;
        src.axis_manager.axis_config(0).range_min  = -1.05;
        src.axis_manager.axis_config(0).range_max  =  1.05;

        REQUIRE(save_session(src, k_sys_tmp));
    }

    // Restore into a fresh state
    AppState dst;
    REQUIRE(load_session(dst, k_sys_tmp));

    REQUIRE(dst.panes.file_pane_visible   == false);
    REQUIRE(dst.panes.signal_pane_visible == true);
    REQUIRE_THAT(dst.panes.file_pane_width,
                 Catch::Matchers::WithinAbs(350.0f, 1e-4f));
    REQUIRE_THAT(dst.panes.signal_pane_width,
                 Catch::Matchers::WithinAbs(420.0f, 1e-4f));
    REQUIRE(dst.axis_manager.axis_config(0).label == "Quaternion [-]");
    REQUIRE(dst.axis_manager.axis_config(0).auto_range == false);
    REQUIRE_THAT(dst.axis_manager.axis_config(0).range_min,
                 Catch::Matchers::WithinAbs(-1.05, 1e-9));
    REQUIRE_THAT(dst.axis_manager.axis_config(0).range_max,
                 Catch::Matchers::WithinAbs( 1.05, 1e-9));

    remove_sys_tmp();
}

// ── 5. Session: plotted signals skipped when sim file is not available ────────
//
// Exercises the load path when a session JSON references simulation files that
// no longer exist.  The signals must be silently skipped and the load must
// return true (the pane state is still restored).

TEST_CASE("System: Session load skips plotted signals when sim file is missing",
          "[system][session]")
{
    remove_sys_tmp();

    // Manually write a session JSON that references a nonexistent HDF5 path
    {
        std::ofstream ofs(k_sys_tmp);
        REQUIRE(ofs.is_open());
        ofs << R"({
  "version": 1,
  "simulations": [
    { "path": "/does/not/exist.h5", "sim_id": "ghost_sim" }
  ],
  "plotted_signals": [
    {
      "sim_id": "ghost_sim",
      "h5_path": "/attitude/quaternion",
      "alias": "q",
      "color_rgba": "0x4477AAFF",
      "y_axis": 0,
      "visible": true,
      "component_index": -1
    }
  ],
  "y_axes": [],
  "annotations": [],
  "panes": {
    "file_pane_visible": true,
    "signal_pane_visible": true,
    "file_pane_width": 280.0,
    "signal_pane_width": 300.0
  }
})";
    }

    AppState state;
    // load_session must not throw and must return true (pane state was loaded)
    bool loaded = false;
    REQUIRE_NOTHROW(loaded = load_session(state, k_sys_tmp));
    REQUIRE(loaded);

    // Ghost sim failed to open → not added to simulations
    REQUIRE(state.simulations.empty());
    // Ghost signal skipped because its sim was not found
    REQUIRE(state.plotted_signals.empty());
    // Pane state should have been restored
    REQUIRE_THAT(state.panes.file_pane_width,
                 Catch::Matchers::WithinAbs(280.0f, 1e-4f));

    remove_sys_tmp();
}
