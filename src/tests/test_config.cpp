#include <catch2/catch_test_macros.hpp>
#include "gnc_viz/config.hpp"
#include <filesystem>
#include <fstream>

using gnc_viz::Config;
using gnc_viz::Palette;

TEST_CASE("Config default-constructs with sensible values", "[config]")
{
    Config c;
    REQUIRE(c.file_pane_width   == 280.0f);
    REQUIRE(c.signal_pane_width == 300.0f);
    REQUIRE(c.font_size         == 14.0f);
    REQUIRE(c.dark_theme        == true);
    REQUIRE(c.restore_session   == true);
}

TEST_CASE("Palette has exactly 20 colors", "[config]")
{
    Palette p;
    REQUIRE(p.colors.size() == 20u);
    // All default colors are non-zero
    for (auto c : p.colors)
        REQUIRE(c != 0u);
}

TEST_CASE("Config round-trips through JSON save/load", "[config]")
{
    namespace fs = std::filesystem;

    Config original;
    original.file_pane_width   = 350.0f;
    original.signal_pane_width = 250.0f;
    original.font_size         = 16.0f;
    original.dark_theme        = false;

    // Write to a temp file
    fs::path tmp = fs::temp_directory_path() / "gnc_viz_config_test.json";
    REQUIRE(original.save(tmp));

    Config loaded = Config::load(tmp);
    REQUIRE(loaded.file_pane_width   == original.file_pane_width);
    REQUIRE(loaded.signal_pane_width == original.signal_pane_width);
    REQUIRE(loaded.font_size         == original.font_size);
    REQUIRE(loaded.dark_theme        == original.dark_theme);

    fs::remove(tmp);
}

TEST_CASE("Config::load returns defaults for non-existent file", "[config]")
{
    Config c = Config::load("/tmp/gnc_viz_no_such_file_xyz.json");
    REQUIRE(c.file_pane_width == 280.0f);
}

TEST_CASE("Config::default_path returns a path ending in config.json", "[config]")
{
    auto p = Config::default_path();
    REQUIRE(p.filename() == "config.json");
}
