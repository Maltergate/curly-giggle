// png_exporter.cpp — Export a PNG screenshot via macOS screencapture

#include "gnc_viz/png_exporter.hpp"
#include "gnc_viz/error.hpp"
#include "gnc_viz/log.hpp"

#include <cstdlib>
#include <format>
#include <string>

namespace gnc_viz {

gnc::Result<bool> export_png(const std::string& /*window_title*/,
                              const std::filesystem::path& path)
{
    // screencapture -i enters interactive crosshair mode so the user can
    // select exactly the region they want to capture.
    const std::string cmd =
        std::format("screencapture -i -t png \"{}\"", path.string());

    GNC_LOG_INFO("PNG export: {}", cmd);
    const int ret = std::system(cmd.c_str()); // NOLINT(cert-env33-c)
    if (ret != 0)
        return gnc::make_error<bool>(
            gnc::ErrorCode::IOError,
            "screencapture failed with exit code " + std::to_string(ret));

    return true;
}

} // namespace gnc_viz
