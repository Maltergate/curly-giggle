#include <catch2/catch_test_macros.hpp>
#include "gnc_viz/log.hpp"
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

TEST_CASE("log::init initialises the logger", "[log]")
{
    gnc_viz::log::init({.console = false, .file = false});
    auto logger = gnc_viz::log::get();
    REQUIRE(logger != nullptr);
    REQUIRE(logger->name() == "gnc_viz");
}

TEST_CASE("log::get returns non-null after init", "[log]")
{
    gnc_viz::log::init({.console = false, .file = false});
    REQUIRE(gnc_viz::log::get() != nullptr);
}

TEST_CASE("GNC_LOG_INFO macro compiles and runs without throw", "[log]")
{
    gnc_viz::log::init({.console = false, .file = false});
    REQUIRE_NOTHROW(GNC_LOG_INFO("test info message {}", 42));
}

TEST_CASE("GNC_LOG_WARN macro works", "[log]")
{
    gnc_viz::log::init({.console = false, .file = false});
    REQUIRE_NOTHROW(GNC_LOG_WARN("test warning: {}", "hello"));
}

TEST_CASE("GNC_LOG_ERROR macro works", "[log]")
{
    gnc_viz::log::init({.console = false, .file = false});
    REQUIRE_NOTHROW(GNC_LOG_ERROR("test error {}", 99));
}

TEST_CASE("log::shutdown does not throw", "[log]")
{
    gnc_viz::log::init({.console = false, .file = false});
    REQUIRE_NOTHROW(gnc_viz::log::shutdown());
}
