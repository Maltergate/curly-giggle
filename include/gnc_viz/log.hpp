#pragma once
/// @file log.hpp
/// @brief Structured logging via spdlog with GNC_LOG_* convenience macros.
/// @ingroup utilities

#include <spdlog/spdlog.h>
#include <string>

namespace gnc_viz::log {

/// @brief Logger configuration: file path, sinks, and log level.
struct Config {
    std::string file_path    = "gnc_viz.log";
    bool        console      = true;
    bool        file         = true;
    spdlog::level::level_enum level = spdlog::level::debug;
};

/// @brief Initialise the "gnc_viz" logger with console and/or file sinks.
/// @param cfg Logger configuration. Safe to call multiple times (no-op after first).
void init(Config cfg = {});

/// @brief Flush all sinks and drop the logger. Call before app exit.
void shutdown();

/// @brief Retrieve the shared logger (valid after init()).
/// @return Shared pointer to the spdlog logger instance.
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
