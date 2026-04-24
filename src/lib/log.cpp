#include "fastscope/log.hpp"
#include "fastscope/ui_log_sink.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <filesystem>
#include <mutex>
#include <vector>

namespace fastscope::log {

namespace {
    std::shared_ptr<spdlog::logger> g_logger;
    std::once_flag                  g_init_flag;
}

void init(Config cfg)
{
    std::call_once(g_init_flag, [&cfg]() {
        std::vector<spdlog::sink_ptr> sinks;

        if (cfg.console) {
            auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console->set_level(cfg.level);
            sinks.push_back(std::move(console));
        }

        if (cfg.file) {
            // Ensure parent directory exists
            auto dir = std::filesystem::path(cfg.file_path).parent_path();
            if (!dir.empty())
                std::filesystem::create_directories(dir);

            constexpr std::size_t max_size  = 10ULL * 1024 * 1024;  // 10 MB per file
            constexpr int         max_files = 3;
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                cfg.file_path, max_size, max_files);
            file_sink->set_level(cfg.level);
            sinks.push_back(std::move(file_sink));
        }

        // ── In-memory UI sink (always registered) ─────────────────────────────
        {
            auto ui_sink = std::make_shared<UILogSink>();
            ui_sink->set_level(spdlog::level::info);   // show info and above in the log pane
            register_ui_log_sink(ui_sink);
            sinks.push_back(std::move(ui_sink));
        }

        g_logger = std::make_shared<spdlog::logger>("fastscope", sinks.begin(), sinks.end());
        g_logger->set_level(cfg.level);
        g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
        spdlog::register_logger(g_logger);
        spdlog::set_default_logger(g_logger);
    });
}

void shutdown()
{
    if (g_logger)
        g_logger->flush();
    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger> get()
{
    return g_logger ? g_logger : spdlog::default_logger();
}

} // namespace fastscope::log
