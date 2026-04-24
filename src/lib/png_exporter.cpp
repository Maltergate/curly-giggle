// png_exporter.cpp — Export a PNG screenshot via macOS screencapture

#include "fastscope/png_exporter.hpp"
#include "fastscope/error.hpp"
#include "fastscope/log.hpp"

#include <cstdlib>
#include <format>
#include <string>

namespace fastscope {

fastscope::Result<bool> export_png(const std::string& /*window_title*/,
                              const std::filesystem::path& path)
{
    // screencapture -i enters interactive crosshair mode so the user can
    // select exactly the region they want to capture.
    const std::string cmd =
        std::format("screencapture -i -t png \"{}\"", path.string());

    FASTSCOPE_LOG_INFO("PNG export: {}", cmd);
    const int ret = std::system(cmd.c_str()); // NOLINT(cert-env33-c)
    if (ret != 0)
        return fastscope::make_error<bool>(
            fastscope::ErrorCode::IOError,
            "screencapture failed with exit code " + std::to_string(ret));

    return true;
}

} // namespace fastscope
