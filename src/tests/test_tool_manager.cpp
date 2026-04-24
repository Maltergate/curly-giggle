// test_tool_manager.cpp — unit tests for ToolManager
// NOTE: No ImGui context is created here.
//   - ToolManager constructor only registers factories (no instances created).
//   - activate() instantiates the tool but calls no ImGui API.
//   - tick() / handle_input() / render_overlay() are NOT called here.

#include <catch2/catch_test_macros.hpp>

#include "fastscope/tool_manager.hpp"

using namespace fastscope;

TEST_CASE("ToolManager default state has no active tool", "[tool_manager]")
{
    ToolManager tm;
    REQUIRE(tm.active_tool_id() == "");
    REQUIRE(tm.active_tool() == nullptr);
}

TEST_CASE("ToolManager available_tools returns annotation and ruler", "[tool_manager]")
{
    ToolManager tm;
    const auto& tools = tm.available_tools();
    REQUIRE(tools.size() == 2);
    REQUIRE(tools[0] == "annotation");
    REQUIRE(tools[1] == "ruler");
}

TEST_CASE("ToolManager activate annotation makes it active", "[tool_manager]")
{
    ToolManager tm;
    tm.activate("annotation");
    REQUIRE(tm.is_active("annotation"));
    REQUIRE(tm.active_tool_id() == "annotation");
    REQUIRE(tm.active_tool() != nullptr);
}

TEST_CASE("ToolManager activate same id twice toggles off", "[tool_manager]")
{
    ToolManager tm;
    tm.activate("annotation");
    tm.activate("annotation");   // toggle off
    REQUIRE(!tm.is_active("annotation"));
    REQUIRE(tm.active_tool_id() == "");
    REQUIRE(tm.active_tool() == nullptr);
}

TEST_CASE("ToolManager activate ruler switches from annotation", "[tool_manager]")
{
    ToolManager tm;
    tm.activate("annotation");
    tm.activate("ruler");
    REQUIRE(!tm.is_active("annotation"));
    REQUIRE(tm.is_active("ruler"));
    REQUIRE(tm.active_tool_id() == "ruler");
}

TEST_CASE("ToolManager activate empty string deactivates all", "[tool_manager]")
{
    ToolManager tm;
    tm.activate("annotation");
    tm.activate("");
    REQUIRE(tm.active_tool_id() == "");
    REQUIRE(!tm.is_active("annotation"));
    REQUIRE(tm.active_tool() == nullptr);
}

TEST_CASE("ToolManager activate unknown id does not crash and stays deactivated", "[tool_manager]")
{
    ToolManager tm;
    REQUIRE_NOTHROW(tm.activate("no_such_tool_xyz"));
    REQUIRE(tm.active_tool_id() == "");
    REQUIRE(tm.active_tool() == nullptr);
}

TEST_CASE("ToolManager deactivate after activate clears active tool", "[tool_manager]")
{
    ToolManager tm;
    tm.activate("annotation");
    REQUIRE(tm.is_active("annotation"));
    tm.deactivate();
    REQUIRE(!tm.is_active("annotation"));
    REQUIRE(tm.active_tool_id() == "");
    REQUIRE(tm.active_tool() == nullptr);
}

TEST_CASE("ToolManager deactivate on empty state does not crash", "[tool_manager]")
{
    ToolManager tm;
    REQUIRE_NOTHROW(tm.deactivate());
    REQUIRE(tm.active_tool_id() == "");
}

TEST_CASE("ToolManager activate ruler and deactivate", "[tool_manager]")
{
    ToolManager tm;
    tm.activate("ruler");
    REQUIRE(tm.is_active("ruler"));
    tm.deactivate();
    REQUIRE(!tm.is_active("ruler"));
}
