#include <catch2/catch_test_macros.hpp>
#include "gnc_viz/signal_buffer.hpp"
#include "gnc_viz/signal_metadata.hpp"

using gnc_viz::SignalBuffer;
using gnc_viz::SignalMetadata;

static SignalMetadata make_scalar_meta()
{
    SignalMetadata m;
    m.name    = "alt";
    m.h5_path = "/nav/altitude";
    m.shape   = {5};
    return m;
}

TEST_CASE("SignalBuffer default-constructs as empty", "[signal_buffer]")
{
    SignalBuffer b;
    REQUIRE(!b.is_loaded());
    REQUIRE(b.sample_count() == 0u);
    REQUIRE(b.n_components()  == 1u);
}

TEST_CASE("make_scalar factory produces correct buffer", "[signal_buffer]")
{
    auto meta = make_scalar_meta();
    std::vector<double> t = {0,1,2,3,4};
    std::vector<double> v = {10,20,30,40,50};
    auto buf = SignalBuffer::make_scalar(meta, t, v);

    REQUIRE(buf->is_loaded());
    REQUIRE(buf->sample_count() == 5u);
    REQUIRE(buf->n_components()  == 1u);
    REQUIRE(buf->time().size()   == 5u);
    REQUIRE(buf->values().size() == 5u);
}

TEST_CASE("SignalBuffer::at returns correct value", "[signal_buffer]")
{
    auto meta = make_scalar_meta();
    auto buf = SignalBuffer::make_scalar(meta, {0,1,2}, {7.0,8.0,9.0});
    REQUIRE(buf->at(0) == 7.0);
    REQUIRE(buf->at(1) == 8.0);
    REQUIRE(buf->at(2) == 9.0);
}

TEST_CASE("SignalBuffer::component extracts correct column", "[signal_buffer]")
{
    SignalMetadata meta;
    meta.shape = {3, 2};
    // 3 samples, 2 components: values = {x0,y0, x1,y1, x2,y2}
    std::vector<double> t = {0,1,2};
    std::vector<double> v = {1.0, 2.0,  3.0, 4.0,  5.0, 6.0};
    auto buf = SignalBuffer::make_vector(meta, t, v, 2);

    auto comp0 = buf->component(0);
    auto comp1 = buf->component(1);
    REQUIRE(comp0 == std::vector<double>{1.0, 3.0, 5.0});
    REQUIRE(comp1 == std::vector<double>{2.0, 4.0, 6.0});
}

TEST_CASE("SignalBuffer::evict frees data", "[signal_buffer]")
{
    auto buf = SignalBuffer::make_scalar(make_scalar_meta(), {0,1,2}, {1,2,3});
    REQUIRE(buf->is_loaded());
    buf->evict();
    REQUIRE(!buf->is_loaded());
    REQUIRE(buf->sample_count() == 0u);
}

TEST_CASE("SignalBuffer constructor throws on size mismatch", "[signal_buffer]")
{
    SignalMetadata meta;
    meta.shape = {3};
    // 3 time samples but only 2 values → should throw
    REQUIRE_THROWS_AS(SignalBuffer(meta, {0,1,2}, {1,2}), std::invalid_argument);
}

TEST_CASE("make_vector factory with 3 components", "[signal_buffer]")
{
    SignalMetadata meta;
    meta.name  = "pos";
    meta.shape = {2, 3};
    std::vector<double> t = {0.0, 1.0};
    std::vector<double> v = {1.0, 2.0, 3.0,   // sample 0: XYZ
                              4.0, 5.0, 6.0};  // sample 1: XYZ
    auto buf = SignalBuffer::make_vector(meta, t, v, 3);
    REQUIRE(buf->n_components()  == 3u);
    REQUIRE(buf->sample_count()  == 2u);
    REQUIRE(buf->at(0, 0)        == 1.0);
    REQUIRE(buf->at(1, 2)        == 6.0);
}
