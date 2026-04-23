#include <catch2/catch_test_macros.hpp>
#include "gnc_viz/error.hpp"
#include <string>

using namespace gnc;

// ── Error construction ─────────────────────────────────────────────────────────

TEST_CASE("Error::make constructs correctly", "[error]") {
    auto e = Error::make(ErrorCode::FileNotFound, "not found", "/path/to/file.h5");
    REQUIRE(e.code == ErrorCode::FileNotFound);
    REQUIRE(e.message == "not found");
    REQUIRE(e.context == "/path/to/file.h5");
}

TEST_CASE("Error::to_string includes code name and message", "[error]") {
    auto e = Error::make(ErrorCode::HDF5ReadFailed, "read error");
    auto s = e.to_string();
    REQUIRE(s.contains("HDF5ReadFailed"));
    REQUIRE(s.contains("read error"));
}

TEST_CASE("Error::to_string includes context when present", "[error]") {
    auto e = Error::make(ErrorCode::FileNotFound, "msg", "ctx");
    REQUIRE(e.to_string().contains("ctx"));
}

TEST_CASE("Error::to_string omits brackets when context is empty", "[error]") {
    auto e = Error::make(ErrorCode::FileNotFound, "msg");
    REQUIRE(!e.to_string().contains("["));
}

// ── Result<T> (std::expected) usage ───────────────────────────────────────────

TEST_CASE("Result<int> success path", "[error][result]") {
    Result<int> r = 42;
    REQUIRE(r.has_value());
    REQUIRE(r.value() == 42);
    REQUIRE(*r == 42);
}

TEST_CASE("Result<int> error path", "[error][result]") {
    Result<int> r = std::unexpected(Error::make(ErrorCode::Unknown, "oops"));
    REQUIRE(!r.has_value());
    REQUIRE(r.error().code == ErrorCode::Unknown);
    REQUIRE(r.error().message == "oops");
}

TEST_CASE("Result<void> success", "[error][result]") {
    VoidResult r{};
    REQUIRE(r.has_value());
}

TEST_CASE("Result<void> error", "[error][result]") {
    VoidResult r = make_void_error(ErrorCode::FileOpenFailed, "cannot open");
    REQUIRE(!r.has_value());
    REQUIRE(r.error().code == ErrorCode::FileOpenFailed);
}

// ── make_error helper ──────────────────────────────────────────────────────────

TEST_CASE("make_error<T> produces unexpected", "[error]") {
    auto r = make_error<double>(ErrorCode::DimensionMismatch, "shape mismatch");
    REQUIRE(!r.has_value());
    REQUIRE(r.error().code == ErrorCode::DimensionMismatch);
}

// ── Monadic chaining (std::expected API) ──────────────────────────────────────

TEST_CASE("Result::and_then chains on success", "[error][result]") {
    Result<int> r = 10;
    auto doubled = r.and_then([](int v) -> Result<int> { return v * 2; });
    REQUIRE(doubled.value() == 20);
}

TEST_CASE("Result::and_then short-circuits on error", "[error][result]") {
    Result<int> r = std::unexpected(Error::make(ErrorCode::Unknown, "fail"));
    bool called = false;
    auto out = r.and_then([&](int) -> Result<int> { called = true; return 0; });
    REQUIRE(!called);
    REQUIRE(!out.has_value());
}

TEST_CASE("Result::transform maps value", "[error][result]") {
    Result<int> r = 5;
    auto s = r.transform([](int v) { return std::to_string(v); });
    REQUIRE(s.value() == "5");
}

// ── error_code_name ────────────────────────────────────────────────────────────

TEST_CASE("error_code_name returns non-empty string for all codes", "[error]") {
    for (auto code : {
        ErrorCode::Unknown, ErrorCode::FileNotFound, ErrorCode::FileOpenFailed,
        ErrorCode::FileReadFailed, ErrorCode::HDF5OpenFailed, ErrorCode::HDF5ReadFailed,
        ErrorCode::HDF5TypeMismatch, ErrorCode::SignalNotFound,
        ErrorCode::SignalTypeMismatch, ErrorCode::DimensionMismatch,
        ErrorCode::RenderFailed
    }) {
        REQUIRE(!error_code_name(code).empty());
    }
}
