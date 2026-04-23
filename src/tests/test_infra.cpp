#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include "gnc_viz/version.hpp"

// ── Version ────────────────────────────────────────────────────────────────────

TEST_CASE("Version string is non-empty", "[infra][version]") {
    REQUIRE(!std::string_view{gnc_viz::version()}.empty());
}

TEST_CASE("Version constants are consistent", "[infra][version]") {
    REQUIRE(gnc_viz::VERSION_MAJOR == 0);
    REQUIRE(gnc_viz::VERSION_MINOR == 1);
    REQUIRE(gnc_viz::VERSION_PATCH == 0);
    REQUIRE(gnc_viz::VERSION_STRING == "0.1.0");
}

// ── Build-system sanity ────────────────────────────────────────────────────────

TEST_CASE("C++23 features are available", "[infra][cpp23]") {
    // std::expected is a C++23 stdlib feature — if this compiles, C++23 is active
#if __cplusplus >= 202302L
    SUCCEED("Compiled with C++23 or later");
#else
    FAIL("Expected C++23 (__cplusplus = " << __cplusplus << ")");
#endif
}

TEST_CASE("std::string_view works", "[infra][stdlib]") {
    constexpr std::string_view sv = "hello";
    REQUIRE(sv.size() == 5);
    REQUIRE(sv[0] == 'h');
}
