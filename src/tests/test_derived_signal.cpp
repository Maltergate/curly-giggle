// test_derived_signal.cpp — unit tests for DerivedSignal

#include "gnc_viz/derived_signal.hpp"
#include "gnc_viz/signal_ops.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <memory>

using Catch::Matchers::WithinAbs;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::shared_ptr<gnc_viz::SignalBuffer>
make_scalar(std::vector<double> values, std::string name = "s")
{
    const std::size_t N = values.size();
    std::vector<double> time(N);
    for (std::size_t i = 0; i < N; ++i) time[i] = static_cast<double>(i) * 0.1;

    gnc_viz::SignalMetadata meta;
    meta.name  = std::move(name);
    meta.dtype = gnc_viz::DataType::Float64;
    meta.shape = {N};

    return gnc_viz::SignalBuffer::make_scalar(std::move(meta), std::move(time), std::move(values));
}

// A minimal failing operation for error-propagation tests.
class AlwaysFailOp final : public gnc_viz::ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "AlwaysFail"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "always_fail"; }
    [[nodiscard]] int              input_count() const noexcept override { return 1; }

    gnc::Result<std::shared_ptr<gnc_viz::SignalBuffer>>
    execute(std::span<const std::shared_ptr<gnc_viz::SignalBuffer>>) override
    {
        return gnc::make_error<std::shared_ptr<gnc_viz::SignalBuffer>>(
            gnc::ErrorCode::InvalidArgument, "AlwaysFailOp: intentional failure");
    }
};

// A variadic op that accepts ≥ 2 inputs (delegates to MagnitudeOp).
class VariadicTestOp final : public gnc_viz::ISignalOperation {
public:
    [[nodiscard]] std::string_view name()        const noexcept override { return "VariadicTest"; }
    [[nodiscard]] std::string_view id()          const noexcept override { return "variadic_test"; }
    [[nodiscard]] int              input_count() const noexcept override { return -1; }

    gnc::Result<std::shared_ptr<gnc_viz::SignalBuffer>>
    execute(std::span<const std::shared_ptr<gnc_viz::SignalBuffer>> inputs) override
    {
        return gnc_viz::MagnitudeOp{}.execute(inputs);
    }
};

// ── Test 1: null operation → error ────────────────────────────────────────────

TEST_CASE("DerivedSignal: null operation returns error") {
    gnc_viz::DerivedSignal ds;
    ds.id           = "derived_0";
    ds.display_name = "test";
    ds.operation    = nullptr;
    ds.inputs       = {make_scalar({1.0, 2.0})};

    auto res = ds.compute();
    REQUIRE(!res);
    CHECK(res.error().code == gnc::ErrorCode::InvalidState);
}

// ── Test 2: wrong input count → error ─────────────────────────────────────────

TEST_CASE("DerivedSignal: wrong input count returns error") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_1";
    ds.operation = std::make_shared<gnc_viz::AddOp>();  // expects 2
    ds.inputs    = {make_scalar({1.0, 2.0})};           // only 1

    auto res = ds.compute();
    REQUIRE(!res);
    CHECK(res.error().code == gnc::ErrorCode::InvalidArgument);
}

// ── Test 3: valid add operation → correct output ──────────────────────────────

TEST_CASE("DerivedSignal: add two scalar buffers produces correct output") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_2";
    ds.operation = std::make_shared<gnc_viz::AddOp>();
    ds.inputs    = {make_scalar({1.0, 2.0, 3.0}), make_scalar({10.0, 20.0, 30.0})};

    auto res = ds.compute();
    REQUIRE(res);
    REQUIRE((*res)->sample_count() == 3);
    CHECK_THAT((*res)->at(0), WithinAbs(11.0, 1e-9));
    CHECK_THAT((*res)->at(1), WithinAbs(22.0, 1e-9));
    CHECK_THAT((*res)->at(2), WithinAbs(33.0, 1e-9));
}

// ── Test 4: second call returns cached result ──────────────────────────────────

TEST_CASE("DerivedSignal: second compute() returns cached result") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_3";
    ds.operation = std::make_shared<gnc_viz::AddOp>();
    ds.inputs    = {make_scalar({5.0}), make_scalar({3.0})};

    auto first  = ds.compute();
    auto second = ds.compute();

    REQUIRE(first);
    REQUIRE(second);
    // Same pointer — the cached object is returned.
    CHECK((*first).get() == (*second).get());
}

// ── Test 5: invalidate clears cache ───────────────────────────────────────────

TEST_CASE("DerivedSignal: invalidate() clears cache, next compute is fresh") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_4";
    ds.operation = std::make_shared<gnc_viz::AddOp>();
    ds.inputs    = {make_scalar({1.0}), make_scalar({1.0})};

    auto first = ds.compute();
    REQUIRE(first);
    CHECK(ds.is_computed());

    ds.invalidate();
    CHECK(!ds.is_computed());

    auto second = ds.compute();
    REQUIRE(second);
    CHECK(ds.is_computed());
    // A fresh result with the same value.
    CHECK_THAT((*second)->at(0), WithinAbs(2.0, 1e-9));
}

// ── Test 6: is_computed() before and after ────────────────────────────────────

TEST_CASE("DerivedSignal: is_computed() is false before compute, true after") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_5";
    ds.operation = std::make_shared<gnc_viz::MultiplyOp>();
    ds.inputs    = {make_scalar({2.0, 3.0}), make_scalar({4.0, 5.0})};

    CHECK(!ds.is_computed());
    auto res = ds.compute();
    REQUIRE(res);
    CHECK(ds.is_computed());
}

// ── Test 7: magnitude on 3-component vector ───────────────────────────────────

TEST_CASE("DerivedSignal: magnitude of 3D vector returns correct scalar norms") {
    // |[3,4,0]| = 5, |[0,0,1]| = 1
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_6";
    ds.operation = std::make_shared<gnc_viz::MagnitudeOp>();
    ds.inputs    = {make_scalar({3.0, 0.0}),
                    make_scalar({4.0, 0.0}),
                    make_scalar({0.0, 1.0})};

    auto res = ds.compute();
    REQUIRE(res);
    CHECK((*res)->sample_count() == 2);
    CHECK_THAT((*res)->at(0), WithinAbs(5.0, 1e-9));
    CHECK_THAT((*res)->at(1), WithinAbs(1.0, 1e-9));
}

// ── Test 8: variadic op accepts ≥ 2 inputs ────────────────────────────────────

TEST_CASE("DerivedSignal: variadic op (-1) accepts 3 inputs") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_7";
    ds.operation = std::make_shared<VariadicTestOp>();
    ds.inputs    = {make_scalar({1.0}), make_scalar({0.0}), make_scalar({0.0})};

    auto res = ds.compute();
    REQUIRE(res);
    CHECK_THAT((*res)->at(0), WithinAbs(1.0, 1e-9));
}

// ── Test 9: variadic op with only 1 input → error ────────────────────────────

TEST_CASE("DerivedSignal: variadic op with 1 input returns error") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_8";
    ds.operation = std::make_shared<VariadicTestOp>();
    ds.inputs    = {make_scalar({1.0})};

    auto res = ds.compute();
    REQUIRE(!res);
    CHECK(res.error().code == gnc::ErrorCode::InvalidArgument);
}

// ── Test 10: operation execute() failure is propagated ────────────────────────

TEST_CASE("DerivedSignal: propagates error from operation::execute()") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_9";
    ds.operation = std::make_shared<AlwaysFailOp>();
    ds.inputs    = {make_scalar({1.0, 2.0})};

    auto res = ds.compute();
    REQUIRE(!res);
    CHECK(!res.error().message.empty());
    // Cache should NOT be set on failure.
    CHECK(!ds.is_computed());
}

// ── Test 11: id and display_name fields are accessible ───────────────────────

TEST_CASE("DerivedSignal: id and display_name fields are accessible") {
    gnc_viz::DerivedSignal ds;
    ds.id           = "derived_99";
    ds.display_name = "\u2016r_eci\u2016";

    CHECK(ds.id           == "derived_99");
    CHECK(ds.display_name == "\u2016r_eci\u2016");
}

// ── Test 12: subtract op via DerivedSignal ────────────────────────────────────

TEST_CASE("DerivedSignal: subtract operation produces correct output") {
    gnc_viz::DerivedSignal ds;
    ds.id        = "derived_10";
    ds.operation = std::make_shared<gnc_viz::SubtractOp>();
    ds.inputs    = {make_scalar({10.0, 20.0}), make_scalar({3.0, 7.0})};

    auto res = ds.compute();
    REQUIRE(res);
    CHECK_THAT((*res)->at(0), WithinAbs(7.0, 1e-9));
    CHECK_THAT((*res)->at(1), WithinAbs(13.0, 1e-9));
}
