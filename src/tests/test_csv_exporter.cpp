// test_csv_exporter.cpp — Unit tests for fastscope::export_csv

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "fastscope/csv_exporter.hpp"
#include "fastscope/app_state.hpp"
#include "fastscope/plotted_signal.hpp"
#include "fastscope/signal_buffer.hpp"
#include "fastscope/signal_metadata.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using fastscope::AppState;
using fastscope::PlottedSignal;
using fastscope::SignalBuffer;
using fastscope::SignalMetadata;

// ── Helpers ────────────────────────────────────────────────────────────────────

static SignalMetadata make_meta(const std::string& name,
                                const std::string& h5_path,
                                std::vector<std::size_t> shape)
{
    SignalMetadata m;
    m.name    = name;
    m.h5_path = h5_path;
    m.shape   = std::move(shape);
    return m;
}

static PlottedSignal make_scalar_signal(const std::string& name,
                                        std::vector<double> time,
                                        std::vector<double> values)
{
    PlottedSignal ps;
    ps.meta   = make_meta(name, "/" + name, {time.size()});
    ps.buffer = SignalBuffer::make_scalar(ps.meta, std::move(time), std::move(values));
    ps.visible = true;
    return ps;
}

static PlottedSignal make_vector_signal(const std::string& name,
                                        std::vector<double> time,
                                        std::vector<double> values,
                                        std::size_t n_comp)
{
    PlottedSignal ps;
    ps.meta   = make_meta(name, "/" + name, {time.size(), n_comp});
    ps.buffer = SignalBuffer::make_vector(ps.meta, std::move(time),
                                          std::move(values), n_comp);
    ps.visible = true;
    return ps;
}

/// Read all lines from a file into a vector.
static std::vector<std::string> read_lines(const std::filesystem::path& p)
{
    std::ifstream in(p);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line))
        if (!line.empty()) lines.push_back(line);
    return lines;
}

/// Split a CSV line by comma.
static std::vector<std::string> split_csv(const std::string& line)
{
    std::vector<std::string> cols;
    std::istringstream ss(line);
    std::string tok;
    while (std::getline(ss, tok, ','))
        cols.push_back(tok);
    return cols;
}

// Temp file helper — unique path per test in the system temp directory.
static std::filesystem::path tmp_csv(const std::string& tag)
{
    return std::filesystem::temp_directory_path() /
           ("fastscope_test_csv_" + tag + ".csv");
}

// ── Tests ──────────────────────────────────────────────────────────────────────

TEST_CASE("export_csv: empty plotted_signals returns error", "[csv_exporter]")
{
    AppState state;
    auto result = fastscope::export_csv(state, tmp_csv("empty"));
    REQUIRE_FALSE(result.has_value());
    REQUIRE_THAT(result.error().message,
                 Catch::Matchers::ContainsSubstring("No visible signals"));
}

TEST_CASE("export_csv: invisible signal is skipped", "[csv_exporter]")
{
    AppState state;
    auto ps = make_scalar_signal("alt", {0.0, 1.0, 2.0}, {10.0, 20.0, 30.0});
    ps.visible = false;
    state.plotted_signals.push_back(std::move(ps));

    auto result = fastscope::export_csv(state, tmp_csv("invisible"));
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("export_csv: scalar signal writes correct row count", "[csv_exporter]")
{
    AppState state;
    state.plotted_signals.push_back(
        make_scalar_signal("alt", {0.0, 1.0, 2.0, 3.0}, {100.0, 200.0, 300.0, 400.0}));

    auto path = tmp_csv("scalar_rowcount");
    auto result = fastscope::export_csv(state, path);

    REQUIRE(result.has_value());
    REQUIRE(*result == 4u);  // 4 data rows

    auto lines = read_lines(path);
    REQUIRE(lines.size() == 5u);  // 1 header + 4 data rows
    std::filesystem::remove(path);
}

TEST_CASE("export_csv: vector signal (4 components) writes 5 columns", "[csv_exporter]")
{
    // time + 4 component columns = 5 columns
    AppState state;
    std::vector<double> time   = {0.0, 1.0, 2.0};
    std::vector<double> values = {
        1.0, 2.0, 3.0, 4.0,   // sample 0
        5.0, 6.0, 7.0, 8.0,   // sample 1
        9.0,10.0,11.0,12.0    // sample 2
    };
    state.plotted_signals.push_back(
        make_vector_signal("quat", time, values, 4));

    auto path = tmp_csv("vector4");
    auto result = fastscope::export_csv(state, path);

    REQUIRE(result.has_value());
    auto lines = read_lines(path);
    REQUIRE(!lines.empty());
    auto cols = split_csv(lines[0]);  // header
    REQUIRE(cols.size() == 5u);       // time + 4 component columns
    std::filesystem::remove(path);
}

TEST_CASE("export_csv: two signals produce correct column count", "[csv_exporter]")
{
    AppState state;
    state.plotted_signals.push_back(
        make_scalar_signal("alt", {0.0, 1.0, 2.0}, {1.0, 2.0, 3.0}));
    state.plotted_signals.push_back(
        make_scalar_signal("vel", {0.0, 1.0, 2.0}, {4.0, 5.0, 6.0}));

    auto path = tmp_csv("two_signals");
    auto result = fastscope::export_csv(state, path);

    REQUIRE(result.has_value());
    auto lines = read_lines(path);
    REQUIRE(lines.size() == 4u);  // header + 3 rows

    auto cols = split_csv(lines[0]);  // header
    REQUIRE(cols.size() == 3u);       // time + alt + vel
    std::filesystem::remove(path);
}

TEST_CASE("export_csv: header contains correct column names", "[csv_exporter]")
{
    AppState state;
    // scalar signal
    state.plotted_signals.push_back(
        make_scalar_signal("altitude", {0.0, 1.0}, {100.0, 200.0}));
    // vector signal, 3 components
    state.plotted_signals.push_back(
        make_vector_signal("position", {0.0, 1.0},
                           {1.0,2.0,3.0, 4.0,5.0,6.0}, 3));

    auto path = tmp_csv("header_names");
    auto result = fastscope::export_csv(state, path);

    REQUIRE(result.has_value());
    auto lines = read_lines(path);
    REQUIRE(!lines.empty());
    auto cols = split_csv(lines[0]);

    // Expected: time, /altitude, /position[0], /position[1], /position[2]
    REQUIRE(cols.size() == 5u);
    REQUIRE(cols[0] == "time");
    REQUIRE(cols[1] == "/altitude");
    REQUIRE(cols[2] == "/position[0]");
    REQUIRE(cols[3] == "/position[1]");
    REQUIRE(cols[4] == "/position[2]");
    std::filesystem::remove(path);
}

TEST_CASE("export_csv: values in output file are correct", "[csv_exporter]")
{
    AppState state;
    state.plotted_signals.push_back(
        make_scalar_signal("speed", {0.0, 1.0, 2.0}, {10.5, 20.5, 30.5}));

    auto path = tmp_csv("values");
    auto result = fastscope::export_csv(state, path);

    REQUIRE(result.has_value());
    auto lines = read_lines(path);
    REQUIRE(lines.size() == 4u);  // header + 3 data rows

    // Check first data row: time=0, speed=10.5
    auto row0 = split_csv(lines[1]);
    REQUIRE(row0.size() == 2u);
    REQUIRE(std::stod(row0[0]) == 0.0);
    REQUIRE(std::stod(row0[1]) == 10.5);

    // Check last data row: time=2, speed=30.5
    auto row2 = split_csv(lines[3]);
    REQUIRE(std::stod(row2[0]) == 2.0);
    REQUIRE(std::stod(row2[1]) == 30.5);

    std::filesystem::remove(path);
}

TEST_CASE("export_csv: single component_index produces one value column", "[csv_exporter]")
{
    AppState state;
    auto ps = make_vector_signal("gyro", {0.0, 1.0},
                                 {1.0,2.0,3.0, 4.0,5.0,6.0}, 3);
    ps.component_index = 1;  // only export component 1
    state.plotted_signals.push_back(std::move(ps));

    auto path = tmp_csv("single_component");
    auto result = fastscope::export_csv(state, path);

    REQUIRE(result.has_value());
    auto lines = read_lines(path);
    REQUIRE(!lines.empty());

    auto header = split_csv(lines[0]);
    REQUIRE(header.size() == 2u);  // time + one component column
    REQUIRE_THAT(header[1], Catch::Matchers::ContainsSubstring("[1]"));

    auto row0 = split_csv(lines[1]);
    REQUIRE(std::stod(row0[1]) == 2.0);  // component 1 of sample 0
    std::filesystem::remove(path);
}
