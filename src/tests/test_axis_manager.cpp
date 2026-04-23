// test_axis_manager.cpp — unit tests for AxisManager

#include "gnc_viz/axis_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using gnc_viz::AxisManager;
using gnc_viz::AxisConfig;

// ── Default behaviour ─────────────────────────────────────────────────────────

TEST_CASE("AxisManager: unknown key returns axis 0 by default", "[axis_manager]")
{
    AxisManager am;
    REQUIRE(am.get_axis_for("nonexistent_key") == 0);
    REQUIRE(am.get_axis_for("another_key")     == 0);
}

TEST_CASE("AxisManager: default active_axes includes axis 0", "[axis_manager]")
{
    AxisManager am;
    auto axes = am.active_axes();
    REQUIRE(axes.size() >= 1u);
    REQUIRE(axes[0].id == 0);
}

// ── assign / get round-trip ───────────────────────────────────────────────────

TEST_CASE("AxisManager: assign + get_axis_for round-trip", "[axis_manager]")
{
    AxisManager am;
    am.assign("sig_a", 0);
    am.assign("sig_b", 1);
    am.assign("sig_c", 2);

    REQUIRE(am.get_axis_for("sig_a") == 0);
    REQUIRE(am.get_axis_for("sig_b") == 1);
    REQUIRE(am.get_axis_for("sig_c") == 2);
}

TEST_CASE("AxisManager: assign clamps axis_id to [0, max_axes-1]", "[axis_manager]")
{
    AxisManager am;
    am.assign("sig_neg",   -5);
    am.assign("sig_large", 99);

    REQUIRE(am.get_axis_for("sig_neg")   == 0);
    REQUIRE(am.get_axis_for("sig_large") == AxisManager::max_axes - 1);
}

TEST_CASE("AxisManager: reassigning a key updates the axis", "[axis_manager]")
{
    AxisManager am;
    am.assign("sig_x", 0);
    REQUIRE(am.get_axis_for("sig_x") == 0);
    am.assign("sig_x", 2);
    REQUIRE(am.get_axis_for("sig_x") == 2);
}

// ── release ───────────────────────────────────────────────────────────────────

TEST_CASE("AxisManager: release removes assignment, key falls back to 0",
          "[axis_manager]")
{
    AxisManager am;
    am.assign("sig_y", 1);
    REQUIRE(am.get_axis_for("sig_y") == 1);

    am.release("sig_y");
    REQUIRE(am.get_axis_for("sig_y") == 0); // default
}

TEST_CASE("AxisManager: releasing a non-assigned key is a no-op", "[axis_manager]")
{
    AxisManager am;
    am.release("ghost_signal"); // should not throw or crash
    REQUIRE(am.get_axis_for("ghost_signal") == 0);
}

// ── active_axes ───────────────────────────────────────────────────────────────

TEST_CASE("AxisManager: active_axes always includes axis 0 even if no signals",
          "[axis_manager]")
{
    AxisManager am;
    auto axes = am.active_axes();
    REQUIRE(!axes.empty());
    bool has_zero = false;
    for (const auto& a : axes)
        if (a.id == 0) { has_zero = true; break; }
    REQUIRE(has_zero);
}

TEST_CASE("AxisManager: active_axes includes axis 1 only when it has signals",
          "[axis_manager]")
{
    AxisManager am;
    auto axes_before = am.active_axes();
    const std::size_t count_before = axes_before.size();

    am.assign("sig_z", 1);
    auto axes_after = am.active_axes();
    REQUIRE(axes_after.size() == count_before + 1);

    bool has_one = false;
    for (const auto& a : axes_after)
        if (a.id == 1) { has_one = true; break; }
    REQUIRE(has_one);
}

// ── has_signals_on / active_axis_count ───────────────────────────────────────

TEST_CASE("AxisManager: has_signals_on correct behaviour", "[axis_manager]")
{
    AxisManager am;
    REQUIRE(!am.has_signals_on(0));
    REQUIRE(!am.has_signals_on(1));

    am.assign("s1", 0);
    REQUIRE(am.has_signals_on(0));
    REQUIRE(!am.has_signals_on(1));

    am.assign("s2", 1);
    REQUIRE(am.has_signals_on(1));
}

TEST_CASE("AxisManager: active_axis_count counts distinct axes with signals",
          "[axis_manager]")
{
    AxisManager am;
    REQUIRE(am.active_axis_count() == 0);

    am.assign("s1", 0);
    REQUIRE(am.active_axis_count() == 1);

    am.assign("s2", 0); // same axis
    REQUIRE(am.active_axis_count() == 1);

    am.assign("s3", 2);
    REQUIRE(am.active_axis_count() == 2);

    am.release("s1");
    am.release("s2");
    REQUIRE(am.active_axis_count() == 1); // only axis 2 remains
}

// ── clear ─────────────────────────────────────────────────────────────────────

TEST_CASE("AxisManager: clear resets all assignments and configs", "[axis_manager]")
{
    AxisManager am;
    am.assign("s1", 0);
    am.assign("s2", 1);
    am.assign("s3", 2);
    am.axis_config(1).label = "Velocity [m/s]";

    am.clear();

    REQUIRE(am.active_axis_count() == 0);
    REQUIRE(!am.has_signals_on(0));
    REQUIRE(!am.has_signals_on(1));
    REQUIRE(!am.has_signals_on(2));
    // Configs reset
    REQUIRE(am.axis_config(1).label.empty());
    // IDs preserved
    REQUIRE(am.axis_config(0).id == 0);
    REQUIRE(am.axis_config(1).id == 1);
    REQUIRE(am.axis_config(2).id == 2);
}

// ── axis_config mutation ──────────────────────────────────────────────────────

TEST_CASE("AxisManager: axis_config label and units can be mutated", "[axis_manager]")
{
    AxisManager am;
    auto& cfg = am.axis_config(1);
    cfg.label = "Position [km]";
    cfg.units = "km";
    cfg.auto_range = false;
    cfg.range_min  = -100.0;
    cfg.range_max  =  100.0;

    const auto& cfg_read = am.axis_config(1);
    REQUIRE(cfg_read.label      == "Position [km]");
    REQUIRE(cfg_read.units      == "km");
    REQUIRE(cfg_read.auto_range == false);
    REQUIRE(cfg_read.range_min  == -100.0);
    REQUIRE(cfg_read.range_max  ==  100.0);
}

TEST_CASE("AxisManager: axis_config id is preserved after mutation", "[axis_manager]")
{
    AxisManager am;
    am.axis_config(2).label = "Test";
    REQUIRE(am.axis_config(2).id == 2);
}

TEST_CASE("AxisManager: axis_config out-of-range clamps", "[axis_manager]")
{
    AxisManager am;
    // Should not crash; clamps to valid range
    auto& cfg_neg   = am.axis_config(-1);
    auto& cfg_large = am.axis_config(99);

    REQUIRE(cfg_neg.id   == 0);
    REQUIRE(cfg_large.id == AxisManager::max_axes - 1);
}
