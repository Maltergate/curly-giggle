// test_time_aligner.cpp — unit tests for TimeAligner

#include "fastscope/time_aligner.hpp"
#include "fastscope/signal_buffer.hpp"
#include "fastscope/signal_metadata.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <memory>
#include <vector>

using fastscope::TimeAligner;
using fastscope::SignalBuffer;
using fastscope::SignalMetadata;
using Catch::Matchers::WithinAbs;

// ── Helpers ───────────────────────────────────────────────────────────────────

static SignalMetadata make_meta(const std::string& name, std::size_t n)
{
    SignalMetadata m;
    m.name  = name;
    m.shape = {n};
    return m;
}

/// Build a scalar buffer with a uniform time grid.
static std::shared_ptr<SignalBuffer>
make_uniform_scalar(const std::string& name,
                    double t_start, double dt, std::size_t n,
                    double val_start = 0.0, double val_step = 1.0)
{
    std::vector<double> t(n), v(n);
    for (std::size_t i = 0; i < n; ++i) {
        t[i] = t_start + static_cast<double>(i) * dt;
        v[i] = val_start + static_cast<double>(i) * val_step;
    }
    return SignalBuffer::make_scalar(make_meta(name, n), t, v);
}

// ── make_uniform_grid ─────────────────────────────────────────────────────────

TEST_CASE("TimeAligner::make_uniform_grid basic", "[time_aligner]")
{
    auto grid = TimeAligner::make_uniform_grid(0.0, 1.0, 0.1);
    REQUIRE(grid.size() == 11u);
    REQUIRE_THAT(grid.front(), WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(grid.back(),  WithinAbs(1.0, 1e-9));
}

TEST_CASE("TimeAligner::make_uniform_grid single point", "[time_aligner]")
{
    auto grid = TimeAligner::make_uniform_grid(0.5, 0.5, 0.1);
    REQUIRE(grid.size() == 1u);
    REQUIRE_THAT(grid[0], WithinAbs(0.5, 1e-12));
}

TEST_CASE("TimeAligner::make_uniform_grid zero dt returns empty", "[time_aligner]")
{
    auto grid = TimeAligner::make_uniform_grid(0.0, 1.0, 0.0);
    REQUIRE(grid.empty());
}

TEST_CASE("TimeAligner::make_uniform_grid inverted range returns empty", "[time_aligner]")
{
    auto grid = TimeAligner::make_uniform_grid(1.0, 0.0, 0.1);
    REQUIRE(grid.empty());
}

// ── interpolate ───────────────────────────────────────────────────────────────

TEST_CASE("TimeAligner::interpolate exact midpoint", "[time_aligner]")
{
    std::vector<double> st = {0.0, 1.0, 2.0};
    std::vector<double> sv = {0.0, 10.0, 20.0};
    std::vector<double> dt = {0.5, 1.5};

    auto out = TimeAligner::interpolate(st, sv, dt);
    REQUIRE(out.size() == 2u);
    REQUIRE_THAT(out[0], WithinAbs(5.0,  1e-9));
    REQUIRE_THAT(out[1], WithinAbs(15.0, 1e-9));
}

TEST_CASE("TimeAligner::interpolate clamps below range", "[time_aligner]")
{
    std::vector<double> st = {1.0, 2.0, 3.0};
    std::vector<double> sv = {10.0, 20.0, 30.0};
    std::vector<double> dt = {0.0, -5.0}; // both below t=1

    auto out = TimeAligner::interpolate(st, sv, dt);
    REQUIRE(out.size() == 2u);
    REQUIRE_THAT(out[0], WithinAbs(10.0, 1e-9));
    REQUIRE_THAT(out[1], WithinAbs(10.0, 1e-9));
}

TEST_CASE("TimeAligner::interpolate clamps above range", "[time_aligner]")
{
    std::vector<double> st = {0.0, 1.0};
    std::vector<double> sv = {5.0, 15.0};
    std::vector<double> dt = {2.0, 100.0}; // both above t=1

    auto out = TimeAligner::interpolate(st, sv, dt);
    REQUIRE(out.size() == 2u);
    REQUIRE_THAT(out[0], WithinAbs(15.0, 1e-9));
    REQUIRE_THAT(out[1], WithinAbs(15.0, 1e-9));
}

TEST_CASE("TimeAligner::interpolate exact endpoints", "[time_aligner]")
{
    std::vector<double> st = {0.0, 1.0, 2.0};
    std::vector<double> sv = {3.0, 7.0, 11.0};
    std::vector<double> dt = {0.0, 2.0};

    auto out = TimeAligner::interpolate(st, sv, dt);
    REQUIRE_THAT(out[0], WithinAbs(3.0,  1e-9));
    REQUIRE_THAT(out[1], WithinAbs(11.0, 1e-9));
}

// ── align ─────────────────────────────────────────────────────────────────────

TEST_CASE("TimeAligner::align empty input returns empty", "[time_aligner]")
{
    std::vector<std::shared_ptr<SignalBuffer>> inputs;
    auto res = TimeAligner::align(inputs);
    REQUIRE(res);
    REQUIRE(res->empty());
}

TEST_CASE("TimeAligner::align single buffer returns copy unchanged", "[time_aligner]")
{
    auto buf = make_uniform_scalar("alt", 0.0, 0.01, 100, 0.0, 1.0);
    std::vector<std::shared_ptr<SignalBuffer>> inputs{buf};

    auto res = TimeAligner::align(inputs);
    REQUIRE(res);
    REQUIRE(res->size() == 1u);

    const auto& out = (*res)[0];
    REQUIRE(out->sample_count() == buf->sample_count());
    REQUIRE_THAT(out->time()[0],   WithinAbs(buf->time()[0],   1e-12));
    REQUIRE_THAT(out->time()[99],  WithinAbs(buf->time()[99],  1e-12));
    REQUIRE_THAT(out->at(0),  WithinAbs(buf->at(0),  1e-12));
    REQUIRE_THAT(out->at(50), WithinAbs(buf->at(50), 1e-12));
}

TEST_CASE("TimeAligner::align two buffers same time → fast path", "[time_aligner]")
{
    // Build both buffers with identical time vectors
    std::vector<double> t  = {0.0, 0.1, 0.2, 0.3, 0.4};
    std::vector<double> v1 = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> v2 = {10.0, 9.0, 8.0, 7.0, 6.0};

    auto buf1 = SignalBuffer::make_scalar(make_meta("s1", 5), t, v1);
    auto buf2 = SignalBuffer::make_scalar(make_meta("s2", 5), t, v2);

    std::vector<std::shared_ptr<SignalBuffer>> inputs{buf1, buf2};
    auto res = TimeAligner::align(inputs);
    REQUIRE(res);
    REQUIRE(res->size() == 2u);

    const auto& o1 = (*res)[0];
    const auto& o2 = (*res)[1];
    REQUIRE(o1->sample_count() == 5u);
    REQUIRE(o2->sample_count() == 5u);

    // Values must be preserved exactly (no interpolation)
    for (std::size_t i = 0; i < 5; ++i) {
        REQUIRE_THAT(o1->at(i), WithinAbs(v1[i], 1e-12));
        REQUIRE_THAT(o2->at(i), WithinAbs(v2[i], 1e-12));
    }
}

TEST_CASE("TimeAligner::align 100 Hz vs 50 Hz → 100 Hz grid, correct values",
          "[time_aligner]")
{
    // 100 Hz: 0.0 .. 1.0 in steps of 0.01, 101 samples, values = time * 2
    auto buf100 = make_uniform_scalar("sig100", 0.0, 0.01, 101, 0.0, 0.02);
    // 50 Hz : 0.0 .. 1.0 in steps of 0.02,  51 samples, values = 10 + time * 4
    auto buf50  = make_uniform_scalar("sig50",  0.0, 0.02,  51, 10.0, 0.08);

    std::vector<std::shared_ptr<SignalBuffer>> inputs{buf100, buf50};
    auto res = TimeAligner::align(inputs);
    REQUIRE(res);
    REQUIRE(res->size() == 2u);

    const auto& o100 = (*res)[0];
    const auto& o50  = (*res)[1];

    // Both must have the same number of samples
    REQUIRE(o100->sample_count() == o50->sample_count());

    // Grid should be at the 100 Hz resolution (101 points from 0 to 1)
    REQUIRE(o100->sample_count() == 101u);

    // Check start/end times
    REQUIRE_THAT(o100->time()[0],   WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(o100->time()[100], WithinAbs(1.0, 1e-9));

    // sig100: values[i] = 0 + i * 0.02 = i * 0.02, so at t=0.5 (i=50), val=1.0
    REQUIRE_THAT(o100->at(50), WithinAbs(1.0, 1e-9));

    // sig50: at t=0.5 (midpoint of 50 Hz), value = 10 + 0.5 * 4 = 12.0
    REQUIRE_THAT(o50->at(50), WithinAbs(12.0, 1e-9));
}

TEST_CASE("TimeAligner::align non-overlapping ranges returns error", "[time_aligner]")
{
    // buf1: t=[0, 1], buf2: t=[2, 3] — no overlap
    auto buf1 = make_uniform_scalar("a", 0.0, 0.1, 11);
    auto buf2 = make_uniform_scalar("b", 2.0, 0.1, 11);

    std::vector<std::shared_ptr<SignalBuffer>> inputs{buf1, buf2};
    auto res = TimeAligner::align(inputs);
    REQUIRE(!res);
    REQUIRE(res.error().code == fastscope::ErrorCode::InvalidArgument);
}

TEST_CASE("TimeAligner::align vector signals (3 components)", "[time_aligner]")
{
    // 100 Hz vector signal: values = {i, 2*i, 3*i} for sample i
    const std::size_t N = 11;
    std::vector<double> t1(N), v1(N * 3);
    for (std::size_t i = 0; i < N; ++i) {
        t1[i]         = static_cast<double>(i) * 0.1;
        v1[i * 3 + 0] = static_cast<double>(i);
        v1[i * 3 + 1] = static_cast<double>(i) * 2.0;
        v1[i * 3 + 2] = static_cast<double>(i) * 3.0;
    }
    SignalMetadata meta;
    meta.name  = "vec";
    meta.shape = {N, 3};
    auto buf = SignalBuffer::make_vector(meta, t1, v1, 3);

    // 50 Hz scalar: just to force interpolation path
    auto buf_scalar = make_uniform_scalar("sc", 0.0, 0.2, 6, 0.0, 1.0);

    std::vector<std::shared_ptr<SignalBuffer>> inputs{buf, buf_scalar};
    auto res = TimeAligner::align(inputs);
    REQUIRE(res);
    REQUIRE(res->size() == 2u);

    const auto& ovec = (*res)[0];
    REQUIRE(ovec->n_components() == 3u);

    // At t=0.5 (sample index 5 on 100 Hz grid), components should be ~{5,10,15}
    REQUIRE_THAT(ovec->at(5, 0), WithinAbs(5.0,  1e-9));
    REQUIRE_THAT(ovec->at(5, 1), WithinAbs(10.0, 1e-9));
    REQUIRE_THAT(ovec->at(5, 2), WithinAbs(15.0, 1e-9));
}
