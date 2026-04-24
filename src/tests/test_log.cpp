#include <catch2/catch_test_macros.hpp>
#include "fastscope/log.hpp"
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

TEST_CASE("log::init initialises the logger", "[log]")
{
    fastscope::log::init({.console = false, .file = false});
    auto logger = fastscope::log::get();
    REQUIRE(logger != nullptr);
    REQUIRE(logger->name() == "fastscope");
}

TEST_CASE("log::get returns non-null after init", "[log]")
{
    fastscope::log::init({.console = false, .file = false});
    REQUIRE(fastscope::log::get() != nullptr);
}

TEST_CASE("FASTSCOPE_LOG_INFO macro compiles and runs without throw", "[log]")
{
    fastscope::log::init({.console = false, .file = false});
    REQUIRE_NOTHROW(FASTSCOPE_LOG_INFO("test info message {}", 42));
}

TEST_CASE("FASTSCOPE_LOG_WARN macro works", "[log]")
{
    fastscope::log::init({.console = false, .file = false});
    REQUIRE_NOTHROW(FASTSCOPE_LOG_WARN("test warning: {}", "hello"));
}

TEST_CASE("FASTSCOPE_LOG_ERROR macro works", "[log]")
{
    fastscope::log::init({.console = false, .file = false});
    REQUIRE_NOTHROW(FASTSCOPE_LOG_ERROR("test error {}", 99));
}

TEST_CASE("log::shutdown does not throw", "[log]")
{
    fastscope::log::init({.console = false, .file = false});
    REQUIRE_NOTHROW(fastscope::log::shutdown());
}
