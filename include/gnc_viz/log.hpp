#pragma once

// ── GNC Viz structured logging (spdlog) ───────────────────────────────────────
//
// Provides console + rotating-file sinks, named "gnc_viz".
// Uses std::format (SPDLOG_USE_STD_FORMAT=ON) — no bundled fmtlib.
//
// Usage:
//   gnc_viz::log::init();                       // call once at startup
//   GNC_LOG_INFO("Loaded {} signals", count);   // source-location aware
//   GNC_LOG_WARN("HDF5 version mismatch: {}", msg);
//   gnc_viz::log::shutdown();                   // flush + release sinks

#include <spdlog/spdlog.h>
#include <string>

namespace gnc_viz::log {

struct Config {
    std::string file_path    = "gnc_viz.log";
    bool        console      = true;
    bool        file         = true;
    spdlog::level::level_enum level = spdlog::level::debug;
};

/// Initialise the "gnc_viz" logger. Safe to call multiple times (no-op after first).
void init(Config cfg = {});

/// Flush all sinks and drop the logger. Call before app exit.
void shutdown();

/// Retrieve the shared logger (valid after init()).
std::shared_ptr<spdlog::logger> get();

} // namespace gnc_viz::log

// ── Convenience macros ─────────────────────────────────────────────────────────
//
// These forward to spdlog's source-location-aware macros so log lines show
// the correct file:line in output.
//
#define GNC_LOG_TRACE(...)    SPDLOG_LOGGER_TRACE(::gnc_viz::log::get(),    __VA_ARGS__)
#define GNC_LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG(::gnc_viz::log::get(),    __VA_ARGS__)
#define GNC_LOG_INFO(...)     SPDLOG_LOGGER_INFO(::gnc_viz::log::get(),     __VA_ARGS__)
#define GNC_LOG_WARN(...)     SPDLOG_LOGGER_WARN(::gnc_viz::log::get(),     __VA_ARGS__)
#define GNC_LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(::gnc_viz::log::get(),    __VA_ARGS__)
#define GNC_LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::gnc_viz::log::get(), __VA_ARGS__)
