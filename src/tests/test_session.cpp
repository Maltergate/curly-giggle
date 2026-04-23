// test_session.cpp — unit tests for session save / load

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "gnc_viz/session.hpp"
#include "gnc_viz/app_state.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace gnc_viz;

// Shared temp path used across tests (cleaned up by each test that writes it)
static const fs::path k_tmp = fs::temp_directory_path() / "gnc_viz_test_session.json";

static void remove_tmp()
{
    std::error_code ec;
    fs::remove(k_tmp, ec);
}

// ── 1. default_session_path() is under HOME ───────────────────────────────────

TEST_CASE("default_session_path returns path under HOME with gnc_viz component", "[session]")
{
    const fs::path p = default_session_path();

    // Must contain "gnc_viz" somewhere in the path hierarchy
    bool has_gnc_viz = false;
    for (const auto& part : p) {
        if (part.string().find("gnc_viz") != std::string::npos) {
            has_gnc_viz = true;
            break;
        }
    }
    REQUIRE(has_gnc_viz);

    // Must be under HOME (or cwd as fallback)
    const char* home = std::getenv("HOME");
    if (home) {
        fs::path home_path(home);
        // p should start with home_path
        auto rel = fs::relative(p, home_path);
        REQUIRE(!rel.empty());
        REQUIRE(rel.string().find("..") == std::string::npos);
    }
}

// ── 2. save_session with empty AppState creates a file ───────────────────────

TEST_CASE("save_session with empty AppState creates a valid JSON file", "[session]")
{
    remove_tmp();
    AppState state;
    REQUIRE(save_session(state, k_tmp));
    REQUIRE(fs::exists(k_tmp));
    remove_tmp();
}

// ── 3. Saved JSON contains "version": 1 ──────────────────────────────────────

TEST_CASE("saved JSON contains version 1", "[session]")
{
    remove_tmp();
    AppState state;
    REQUIRE(save_session(state, k_tmp));

    std::ifstream ifs(k_tmp);
    REQUIRE(ifs.is_open());
    auto j = nlohmann::json::parse(ifs);
    REQUIRE(j.value("version", 0) == 1);
    remove_tmp();
}

// ── 4. Saved JSON has required top-level keys ─────────────────────────────────

TEST_CASE("saved JSON has simulations, plotted_signals, and panes keys", "[session]")
{
    remove_tmp();
    AppState state;
    REQUIRE(save_session(state, k_tmp));

    std::ifstream ifs(k_tmp);
    auto j = nlohmann::json::parse(ifs);

    REQUIRE(j.contains("simulations"));
    REQUIRE(j.contains("plotted_signals"));
    REQUIRE(j.contains("panes"));
    remove_tmp();
}

// ── 5. load_session with non-existent file returns false ─────────────────────

TEST_CASE("load_session with non-existent file returns false", "[session]")
{
    remove_tmp();                 // make sure it really is absent
    AppState state;
    REQUIRE_FALSE(load_session(state, k_tmp));
}

// ── 6. load_session with invalid JSON returns false and does not throw ────────

TEST_CASE("load_session with invalid JSON returns false without throwing", "[session]")
{
    remove_tmp();
    {
        std::ofstream ofs(k_tmp);
        ofs << "{ this is NOT valid json {{{{";
    }

    AppState state;
    REQUIRE_NOTHROW([&]{ REQUIRE_FALSE(load_session(state, k_tmp)); }());
    remove_tmp();
}

// ── 7. Round-trip: panes.file_pane_width is preserved ────────────────────────

TEST_CASE("save then load restores panes.file_pane_width", "[session]")
{
    remove_tmp();
    {
        AppState src;
        src.panes.file_pane_width = 420.0f;
        REQUIRE(save_session(src, k_tmp));
    }

    AppState dst;
    REQUIRE(load_session(dst, k_tmp));
    REQUIRE_THAT(dst.panes.file_pane_width,
                 Catch::Matchers::WithinAbs(420.0f, 1e-4f));
    remove_tmp();
}

// ── 8. Round-trip: plotted_signals count is 0 when nothing was plotted ────────

TEST_CASE("save then load with no plotted signals gives empty plotted_signals", "[session]")
{
    remove_tmp();
    {
        AppState src;
        REQUIRE(save_session(src, k_tmp));
    }

    AppState dst;
    REQUIRE(load_session(dst, k_tmp));
    REQUIRE(dst.plotted_signals.empty());
    remove_tmp();
}

// ── 9. Round-trip: pane visibility flags are preserved ────────────────────────

TEST_CASE("save then load restores pane visibility flags", "[session]")
{
    remove_tmp();
    {
        AppState src;
        src.panes.file_pane_visible   = false;
        src.panes.signal_pane_visible = false;
        REQUIRE(save_session(src, k_tmp));
    }

    AppState dst;
    REQUIRE(load_session(dst, k_tmp));
    REQUIRE_FALSE(dst.panes.file_pane_visible);
    REQUIRE_FALSE(dst.panes.signal_pane_visible);
    remove_tmp();
}

// ── 10. Round-trip: signal_pane_width is preserved ───────────────────────────

TEST_CASE("save then load restores panes.signal_pane_width", "[session]")
{
    remove_tmp();
    {
        AppState src;
        src.panes.signal_pane_width = 500.0f;
        REQUIRE(save_session(src, k_tmp));
    }

    AppState dst;
    REQUIRE(load_session(dst, k_tmp));
    REQUIRE_THAT(dst.panes.signal_pane_width,
                 Catch::Matchers::WithinAbs(500.0f, 1e-4f));
    remove_tmp();
}
