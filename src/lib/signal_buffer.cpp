#include "gnc_viz/signal_buffer.hpp"
#include <stdexcept>

namespace gnc_viz {

SignalBuffer::SignalBuffer(SignalMetadata meta,
                           std::vector<double> time,
                           std::vector<double> values)
    : m_meta(std::move(meta))
    , m_time(std::move(time))
    , m_values(std::move(values))
    , m_n_comp(m_meta.component_count() > 0 ? m_meta.component_count() : 1)
{
    if (!m_time.empty() && m_values.size() != m_time.size() * m_n_comp) {
        throw std::invalid_argument(
            "SignalBuffer: values.size() must equal time.size() * n_components");
    }
}

std::vector<double> SignalBuffer::component(std::size_t comp_idx) const
{
    if (comp_idx >= m_n_comp)
        throw std::out_of_range("component index out of range");

    const std::size_t N = m_time.size();
    std::vector<double> out(N);
    for (std::size_t i = 0; i < N; ++i)
        out[i] = m_values[i * m_n_comp + comp_idx];
    return out;
}

std::shared_ptr<SignalBuffer>
SignalBuffer::make_scalar(SignalMetadata meta,
                          std::vector<double> time,
                          std::vector<double> values)
{
    meta.shape = {time.size()};
    return std::make_shared<SignalBuffer>(
        std::move(meta), std::move(time), std::move(values));
}

std::shared_ptr<SignalBuffer>
SignalBuffer::make_vector(SignalMetadata meta,
                          std::vector<double> time,
                          std::vector<double> values,
                          std::size_t n_components)
{
    meta.shape = {time.size(), n_components};
    auto buf = std::make_shared<SignalBuffer>();
    buf->m_meta    = std::move(meta);
    buf->m_time    = std::move(time);
    buf->m_values  = std::move(values);
    buf->m_n_comp  = n_components;

    if (!buf->m_time.empty() &&
        buf->m_values.size() != buf->m_time.size() * n_components)
    {
        throw std::invalid_argument(
            "SignalBuffer::make_vector: values size mismatch");
    }
    return buf;
}

} // namespace gnc_viz
