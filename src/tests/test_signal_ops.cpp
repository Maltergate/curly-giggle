// test_signal_ops.cpp — unit tests for AddOp, SubtractOp, MultiplyOp, ScaleOp, MagnitudeOp

#include "fastscope/signal_ops.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using Catch::Matchers::WithinAbs;

static std::shared_ptr<fastscope::SignalBuffer>
make_signal(std::vector<double> values, std::string name = "s")
{
    const std::size_t N = values.size();
    std::vector<double> time(N);
    for (std::size_t i = 0; i < N; ++i) time[i] = static_cast<double>(i) * 0.1;

    fastscope::SignalMetadata meta;
    meta.name  = std::move(name);
    meta.dtype = fastscope::DataType::Float64;
    meta.shape = {N};

    return fastscope::SignalBuffer::make_scalar(std::move(meta), std::move(time), std::move(values));
}

// ── AddOp ─────────────────────────────────────────────────────────────────────

TEST_CASE("AddOp: basic element-wise addition") {
    auto a = make_signal({1.0, 2.0, 3.0}, "a");
    auto b = make_signal({4.0, 5.0, 6.0}, "b");

    fastscope::AddOp op;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 2> in = {a, b};
    auto res = op.execute(in);
    REQUIRE(res);
    REQUIRE((*res)->sample_count() == 3);
    REQUIRE_THAT((*res)->at(0), WithinAbs(5.0, 1e-9));
    REQUIRE_THAT((*res)->at(1), WithinAbs(7.0, 1e-9));
    REQUIRE_THAT((*res)->at(2), WithinAbs(9.0, 1e-9));
}

TEST_CASE("AddOp: wrong input count returns error") {
    auto a = make_signal({1.0, 2.0});
    fastscope::AddOp op;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 1> in = {a};
    auto res = op.execute(in);
    REQUIRE(!res);
}

TEST_CASE("AddOp: mismatched lengths return error") {
    auto a = make_signal({1.0, 2.0, 3.0});
    auto b = make_signal({1.0, 2.0});
    fastscope::AddOp op;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 2> in = {a, b};
    auto res = op.execute(in);
    REQUIRE(!res);
}

// ── SubtractOp ────────────────────────────────────────────────────────────────

TEST_CASE("SubtractOp: element-wise subtraction") {
    auto a = make_signal({10.0, 20.0, 30.0});
    auto b = make_signal({ 3.0,  5.0,  7.0});

    fastscope::SubtractOp op;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 2> in = {a, b};
    auto res = op.execute(in);
    REQUIRE(res);
    REQUIRE_THAT((*res)->at(0), WithinAbs( 7.0, 1e-9));
    REQUIRE_THAT((*res)->at(1), WithinAbs(15.0, 1e-9));
    REQUIRE_THAT((*res)->at(2), WithinAbs(23.0, 1e-9));
}

// ── MultiplyOp ────────────────────────────────────────────────────────────────

TEST_CASE("MultiplyOp: element-wise multiplication") {
    auto a = make_signal({2.0, 3.0, 4.0});
    auto b = make_signal({5.0, 6.0, 7.0});

    fastscope::MultiplyOp op;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 2> in = {a, b};
    auto res = op.execute(in);
    REQUIRE(res);
    REQUIRE_THAT((*res)->at(0), WithinAbs(10.0, 1e-9));
    REQUIRE_THAT((*res)->at(1), WithinAbs(18.0, 1e-9));
    REQUIRE_THAT((*res)->at(2), WithinAbs(28.0, 1e-9));
}

// ── ScaleOp ───────────────────────────────────────────────────────────────────

TEST_CASE("ScaleOp: scales all samples by factor") {
    auto a = make_signal({1.0, 2.0, 3.0});

    fastscope::ScaleOp op;
    op.scale_factor = 2.5;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 1> in = {a};
    auto res = op.execute(in);
    REQUIRE(res);
    REQUIRE_THAT((*res)->at(0), WithinAbs(2.5, 1e-9));
    REQUIRE_THAT((*res)->at(1), WithinAbs(5.0, 1e-9));
    REQUIRE_THAT((*res)->at(2), WithinAbs(7.5, 1e-9));
}

TEST_CASE("ScaleOp: scale by zero gives zeros") {
    auto a = make_signal({1.0, 2.0, 3.0});
    fastscope::ScaleOp op;
    op.scale_factor = 0.0;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 1> in = {a};
    auto res = op.execute(in);
    REQUIRE(res);
    REQUIRE_THAT((*res)->at(0), WithinAbs(0.0, 1e-9));
}

// ── MagnitudeOp ───────────────────────────────────────────────────────────────

TEST_CASE("MagnitudeOp: 3-D magnitude") {
    // |[3, 4, 0]| = 5,  |[1, 2, 2]| = 3
    auto x = make_signal({3.0, 1.0});
    auto y = make_signal({4.0, 2.0});
    auto z = make_signal({0.0, 2.0});

    fastscope::MagnitudeOp op;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 3> in = {x, y, z};
    auto res = op.execute(in);
    REQUIRE(res);
    REQUIRE_THAT((*res)->at(0), WithinAbs(5.0, 1e-9));
    REQUIRE_THAT((*res)->at(1), WithinAbs(3.0, 1e-9));
}

TEST_CASE("MagnitudeOp: single input returns error") {
    auto a = make_signal({1.0, 2.0});
    fastscope::MagnitudeOp op;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 1> in = {a};
    auto res = op.execute(in);
    REQUIRE(!res);
}

TEST_CASE("MagnitudeOp: preserves time axis") {
    using Catch::Matchers::WithinAbs;
    auto a = make_signal({3.0, 0.0});
    auto b = make_signal({4.0, 5.0});
    fastscope::MagnitudeOp op;
    std::array<std::shared_ptr<fastscope::SignalBuffer>, 2> in = {a, b};
    auto res = op.execute(in);
    REQUIRE(res);
    REQUIRE_THAT((*res)->time()[0], WithinAbs(0.0, 1e-9));
    REQUIRE_THAT((*res)->time()[1], WithinAbs(0.1, 1e-9));
}

// ── Op metadata ───────────────────────────────────────────────────────────────

TEST_CASE("Op IDs and input counts are correct") {
    fastscope::AddOp      add;
    fastscope::SubtractOp sub;
    fastscope::MultiplyOp mul;
    fastscope::ScaleOp    scl;
    fastscope::MagnitudeOp mag;

    CHECK(add.id() == "add");      CHECK(add.input_count() == 2);
    CHECK(sub.id() == "subtract"); CHECK(sub.input_count() == 2);
    CHECK(mul.id() == "multiply"); CHECK(mul.input_count() == 2);
    CHECK(scl.id() == "scale");    CHECK(scl.input_count() == 1);
    CHECK(mag.id() == "magnitude");CHECK(mag.input_count() == -1);
}
