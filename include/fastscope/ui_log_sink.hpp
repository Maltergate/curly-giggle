#pragma once
/// @file ui_log_sink.hpp
/// @brief In-memory spdlog sink that makes recent log entries available to the UI.
/// @ingroup utilities
///
/// UILogSink is registered alongside the console and file sinks during log::init().
/// The UI reads a snapshot of recent entries every frame to render them in the log pane.

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace fastscope {

/// @brief A single log entry stored in the UI ring buffer.
struct UILogEntry {
    spdlog::level::level_enum level = spdlog::level::info;
    std::string time_str;   ///< Formatted as "HH:MM:SS".
    std::string message;    ///< Raw log payload (no level prefix, no timestamp).
};

/// @brief Thread-safe spdlog sink that stores the last MAX_ENTRIES log messages.
/// @details Registered in log::init().  The render thread calls snapshot() once per
///          frame to get a copy for rendering — no locks held during ImGui rendering.
class UILogSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    /// Maximum number of entries retained.
    static constexpr std::size_t MAX_ENTRIES = 500;

    /// @brief Thread-safe copy of current entries.
    /// @return All retained entries, oldest first.
    [[nodiscard]] std::vector<UILogEntry> snapshot();

    /// @brief Monotonically-increasing counter — increments on every new log entry.
    /// @details The render loop compares this to its last-seen value to detect new data
    ///          without taking a lock.
    [[nodiscard]] int generation() const noexcept
    {
        return m_generation.load(std::memory_order_relaxed);
    }

    /// @brief Clear all stored entries and reset the generation counter.
    void clear();

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override {}

private:
    std::deque<UILogEntry>  m_entries;
    std::atomic<int>        m_generation{0};
};

// ── Global accessor ───────────────────────────────────────────────────────────

/// @brief Return the active UILogSink, or nullptr if not yet initialized.
/// @details Valid after log::init().  Do not hold the returned pointer across frames
///          without checking for null.
[[nodiscard]] UILogSink* ui_log_sink() noexcept;

/// @brief Register the given sink as the global UILogSink.
/// @details Called once from log::init().  Not thread-safe — call before any logging.
void register_ui_log_sink(std::shared_ptr<UILogSink> sink);

} // namespace fastscope
