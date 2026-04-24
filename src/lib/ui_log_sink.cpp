#include "fastscope/ui_log_sink.hpp"

#include <chrono>
#include <ctime>
#include <memory>
#include <string>

namespace fastscope {

// ── UILogSink implementation ─────────────────────────────────────────────────

void UILogSink::sink_it_(const spdlog::details::log_msg& msg)
{
    // Format timestamp: HH:MM:SS
    const auto sys_time = std::chrono::time_point_cast<std::chrono::seconds>(msg.time);
    const std::time_t tt = std::chrono::system_clock::to_time_t(sys_time);
    std::tm tm_local{};
#if defined(_WIN32)
    localtime_s(&tm_local, &tt);
#else
    localtime_r(&tt, &tm_local);
#endif
    char time_buf[10];
    std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tm_local);

    UILogEntry entry;
    entry.level    = msg.level;
    entry.time_str = time_buf;
    entry.message  = std::string(msg.payload.data(), msg.payload.size());

    if (m_entries.size() >= MAX_ENTRIES)
        m_entries.pop_front();
    m_entries.push_back(std::move(entry));
    m_generation.fetch_add(1, std::memory_order_relaxed);
}

void UILogSink::clear()
{
    std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
    m_entries.clear();
    m_generation.fetch_add(1, std::memory_order_relaxed);
}

std::vector<UILogEntry> UILogSink::snapshot()
{
    std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
    return std::vector<UILogEntry>(m_entries.begin(), m_entries.end());
}

// ── Global accessor ───────────────────────────────────────────────────────────

namespace {
    std::shared_ptr<UILogSink> g_ui_sink;
}

UILogSink* ui_log_sink() noexcept
{
    return g_ui_sink.get();
}

void register_ui_log_sink(std::shared_ptr<UILogSink> sink)
{
    g_ui_sink = std::move(sink);
}

} // namespace fastscope
