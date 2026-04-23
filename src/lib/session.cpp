#include "gnc_viz/session.hpp"
#include "gnc_viz/log.hpp"

namespace gnc_viz {

std::filesystem::path default_session_path()
{
    const char* home = std::getenv("HOME");
    std::filesystem::path base = home
        ? std::filesystem::path(home)
        : std::filesystem::current_path();
    return base / ".gnc_viz" / "session.json";
}

// ── Stubs (Phase 14 will replace these) ──────────────────────────────────────

bool save_session(const AppState& /*state*/, const std::filesystem::path& path)
{
    GNC_LOG_DEBUG("save_session() stub called (path: {})", path.string());
    return true;  // no-op; considered success
}

bool load_session(AppState& /*state*/, const std::filesystem::path& path)
{
    GNC_LOG_DEBUG("load_session() stub called (path: {})", path.string());
    return false;  // no session to restore (expected until Phase 14)
}

} // namespace gnc_viz
