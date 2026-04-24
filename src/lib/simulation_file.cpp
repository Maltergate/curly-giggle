// simulation_file.cpp — SimulationFile implementation

#include "fastscope/simulation_file.hpp"
#include "fastscope/log.hpp"

namespace fastscope {

SimulationFile::SimulationFile(std::string sim_id)
    : m_sim_id(std::move(sim_id)) {}

fastscope::Result<void> SimulationFile::open(const std::filesystem::path& path)
{
    m_path = path;
    m_display_name = path.filename().string();

    if (auto r = m_reader.open(path.string()); !r)
        return fastscope::make_error<void>(r.error().code, r.error().message);

    auto sigs = m_reader.enumerate_signals(m_sim_id);
    if (!sigs) {
        m_reader.close();
        return fastscope::make_error<void>(sigs.error().code, sigs.error().message);
    }
    m_signals = std::move(*sigs);

    FASTSCOPE_LOG_INFO("SimulationFile[{}]: opened '{}' — {} datasets",
                 m_sim_id, path.filename().string(), m_signals.size());
    return {};
}

void SimulationFile::close()
{
    m_reader.close();
    m_signals.clear();
    m_cache.clear();
}

bool SimulationFile::is_open() const noexcept { return m_reader.is_open(); }

fastscope::Result<std::shared_ptr<SignalBuffer>>
SimulationFile::load_signal(const SignalMetadata& meta)
{
    // Return cached buffer if still alive
    if (auto it = m_cache.find(meta.h5_path); it != m_cache.end())
        if (auto sp = it->second.lock())
            return sp;

    // Load signal — time_path is already set in meta from enumeration
    auto res = m_reader.load_signal(meta);
    if (!res) return res;

    m_cache[meta.h5_path] = *res;
    return res;
}

void SimulationFile::evict_cache() { m_cache.clear(); }

} // namespace fastscope
