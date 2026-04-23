// csv_exporter.cpp — Export plotted signals to CSV

#include "gnc_viz/csv_exporter.hpp"
#include "gnc_viz/app_state.hpp"
#include "gnc_viz/plotted_signal.hpp"
#include "gnc_viz/error.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>

namespace gnc_viz {

gnc::Result<std::size_t> export_csv(const AppState& state,
                                     const std::filesystem::path& path)
{
    // 1. Collect visible signals with loaded buffers.
    std::vector<const PlottedSignal*> signals;
    for (const auto& ps : state.plotted_signals) {
        if (ps.visible && ps.buffer && ps.buffer->is_loaded())
            signals.push_back(&ps);
    }

    if (signals.empty())
        return gnc::make_error<std::size_t>(gnc::ErrorCode::InvalidState,
                                            "No visible signals to export");

    std::ofstream out(path);
    if (!out)
        return gnc::make_error<std::size_t>(gnc::ErrorCode::FileOpenFailed,
                                            "Cannot open file for writing: " + path.string());

    out << std::setprecision(10);

    // 4. Write header: "time" + one column per component per signal.
    out << "time";
    for (const auto* ps : signals) {
        const std::size_t nc = ps->buffer->n_components();
        if (ps->component_index >= 0) {
            // Single component explicitly selected.
            out << ',' << ps->display_name() << '[' << ps->component_index << ']';
        } else if (nc == 1) {
            // Scalar signal.
            out << ',' << ps->display_name();
        } else {
            // All components of a vector signal.
            for (std::size_t k = 0; k < nc; ++k)
                out << ',' << ps->display_name() << '[' << k << ']';
        }
    }
    out << '\n';

    // 5. Write data rows using the first signal's time vector as master.
    const auto master_time = signals[0]->buffer->time();
    std::size_t row_count = 0;

    for (std::size_t i = 0; i < master_time.size(); ++i) {
        const double t = master_time[i];
        out << t;

        for (const auto* ps : signals) {
            const auto& buf = *ps->buffer;
            const auto  sig_time = buf.time();

            // For non-master signals find the nearest sample by time.
            std::size_t idx = i;
            if (ps != signals[0]) {
                auto it = std::lower_bound(sig_time.begin(), sig_time.end(), t);
                if (it == sig_time.end())
                    idx = sig_time.size() - 1;
                else
                    idx = static_cast<std::size_t>(std::distance(sig_time.begin(), it));
                if (idx >= sig_time.size()) idx = sig_time.size() - 1;
            }

            const std::size_t nc = buf.n_components();
            if (ps->component_index >= 0) {
                out << ',' << buf.at(idx, static_cast<std::size_t>(ps->component_index));
            } else if (nc == 1) {
                out << ',' << buf.at(idx, 0);
            } else {
                for (std::size_t k = 0; k < nc; ++k)
                    out << ',' << buf.at(idx, k);
            }
        }
        out << '\n';
        ++row_count;
    }

    return row_count;
}

} // namespace gnc_viz
