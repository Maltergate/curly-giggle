#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "fastscope/color_manager.hpp"

using fastscope::ColorManager;
using fastscope::Palette;
using Catch::Matchers::WithinAbs;

TEST_CASE("ColorManager starts with zero assignments", "[color_manager]")
{
    ColorManager cm;
    REQUIRE(cm.assigned_count() == 0u);
    REQUIRE(cm.palette_size()   == 20u);
}

TEST_CASE("assign returns a non-zero color", "[color_manager]")
{
    ColorManager cm;
    auto c = cm.assign("sig_a");
    REQUIRE(c != 0u);
    REQUIRE(cm.assigned_count() == 1u);
}

TEST_CASE("assign twice returns same color", "[color_manager]")
{
    ColorManager cm;
    auto c1 = cm.assign("sig_a");
    auto c2 = cm.assign("sig_a");
    REQUIRE(c1 == c2);
    REQUIRE(cm.assigned_count() == 1u);
}

TEST_CASE("different signals get different colors (up to palette size)", "[color_manager]")
{
    ColorManager cm;
    auto c1 = cm.assign("sig_a");
    auto c2 = cm.assign("sig_b");
    REQUIRE(c1 != c2);
}

TEST_CASE("release frees a color slot", "[color_manager]")
{
    ColorManager cm;
    cm.assign("sig_a");
    REQUIRE(cm.assigned_count() == 1u);
    cm.release("sig_a");
    REQUIRE(cm.assigned_count() == 0u);
    REQUIRE(cm.get("sig_a") == std::nullopt);
}

TEST_CASE("get returns nullopt for unassigned signal", "[color_manager]")
{
    ColorManager cm;
    REQUIRE(cm.get("no_such") == std::nullopt);
}

TEST_CASE("get_or_default returns fallback for unassigned signal", "[color_manager]")
{
    ColorManager cm;
    auto c = cm.get_or_default("none");
    REQUIRE(c != 0u);
}

TEST_CASE("clear resets all assignments", "[color_manager]")
{
    ColorManager cm;
    cm.assign("a");
    cm.assign("b");
    REQUIRE(cm.assigned_count() == 2u);
    cm.clear();
    REQUIRE(cm.assigned_count() == 0u);
}

TEST_CASE("colors wrap after palette exhausted", "[color_manager]")
{
    ColorManager cm;
    for (int i = 0; i < 25; ++i)
        cm.assign("sig_" + std::to_string(i));
    REQUIRE(cm.assigned_count() == 25u);
    // palette is 20 colors — 21st signal gets palette[0] (wraps)
    auto first  = cm.get("sig_0");
    auto twenty = cm.get("sig_20");
    REQUIRE(first.has_value());
    REQUIRE(twenty.has_value());
    REQUIRE(*first == *twenty);
}

TEST_CASE("to_float4 converts RGBA correctly", "[color_manager]")
{
    // 0xFF0000FF = R=255, G=0, B=0, A=255
    float f[4];
    ColorManager::to_float4(0xFF0000FF, f);
    REQUIRE_THAT(f[0], WithinAbs(1.0f, 0.001f));
    REQUIRE_THAT(f[1], WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(f[2], WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(f[3], WithinAbs(1.0f, 0.001f));
}

TEST_CASE("from_float4 round-trips with to_float4", "[color_manager]")
{
    float r = 0.5f, g = 0.25f, b = 0.75f, a = 1.0f;
    uint32_t packed = ColorManager::from_float4(r, g, b, a);
    float out[4];
    ColorManager::to_float4(packed, out);
    REQUIRE_THAT(out[0], WithinAbs(r, 0.005f));
    REQUIRE_THAT(out[1], WithinAbs(g, 0.005f));
    REQUIRE_THAT(out[2], WithinAbs(b, 0.005f));
    REQUIRE_THAT(out[3], WithinAbs(a, 0.005f));
}
