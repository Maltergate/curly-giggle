#include <catch2/catch_test_macros.hpp>
#include "fastscope/signal_metadata.hpp"

using fastscope::SignalMetadata;
using fastscope::DataType;
using fastscope::data_type_name;

TEST_CASE("SignalMetadata default-constructs", "[signal_metadata]")
{
    SignalMetadata m;
    REQUIRE(m.name.empty());
    REQUIRE(m.h5_path.empty());
    REQUIRE(m.dtype == DataType::Unknown);
    REQUIRE(m.shape.empty());
    REQUIRE(m.sample_count() == 0u);
    REQUIRE(m.component_count() == 1u);
}

TEST_CASE("SignalMetadata::sample_count returns shape[0]", "[signal_metadata]")
{
    SignalMetadata m;
    m.shape = {10000, 4};
    REQUIRE(m.sample_count() == 10000u);
}

TEST_CASE("SignalMetadata::component_count for vector signal", "[signal_metadata]")
{
    SignalMetadata m;
    m.shape = {5000, 3};
    REQUIRE(m.component_count() == 3u);
}

TEST_CASE("SignalMetadata::component_count for scalar signal", "[signal_metadata]")
{
    SignalMetadata m;
    m.shape = {5000};
    REQUIRE(m.component_count() == 1u);
}

TEST_CASE("SignalMetadata::is_scalar", "[signal_metadata]")
{
    SignalMetadata m;
    m.shape = {5000};
    REQUIRE(m.is_scalar());
    REQUIRE(!m.is_vector());
}

TEST_CASE("SignalMetadata::is_vector", "[signal_metadata]")
{
    SignalMetadata m;
    m.shape = {5000, 3};
    REQUIRE(!m.is_scalar());
    REQUIRE(m.is_vector());
}

TEST_CASE("SignalMetadata::unique_key", "[signal_metadata]")
{
    SignalMetadata m;
    m.sim_id  = "sim0";
    m.h5_path = "/attitude/quat";
    REQUIRE(m.unique_key() == "sim0//attitude/quat");
}

TEST_CASE("data_type_name returns correct strings", "[signal_metadata]")
{
    REQUIRE(std::string(data_type_name(DataType::Float64)) == "float64");
    REQUIRE(std::string(data_type_name(DataType::Float32)) == "float32");
    REQUIRE(std::string(data_type_name(DataType::Int32))   == "int32");
    REQUIRE(std::string(data_type_name(DataType::Unknown)) == "unknown");
}

TEST_CASE("SignalMetadata is copyable and movable", "[signal_metadata]")
{
    SignalMetadata a;
    a.name    = "vel_x";
    a.h5_path = "/navigation/velocity";
    a.shape   = {8000};
    a.dtype   = DataType::Float64;

    SignalMetadata b = a;          // copy
    REQUIRE(b.name    == a.name);
    REQUIRE(b.h5_path == a.h5_path);

    SignalMetadata c = std::move(b); // move
    REQUIRE(c.name == "vel_x");
}
