include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ── GLFW (window + input; no OpenGL context — we use Metal) ────────────────────
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW not found via find_package — fetching from source")
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        master   # 3.4 has void* cast errors against macOS SDK 26+
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(glfw)
endif()

# ── Dear ImGui (docking branch — required for multi-viewport / detachable pane) ─
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# imgui_impl_metal.mm must be compiled with ARC (Automatic Reference Counting).
# Without ARC the file leaks: every frame creates FramebufferDescriptor via alloc
# and assigns it to a strong property whose setter retains (+2 total), but the
# original alloc +1 is never balanced by a matching release.  The same MRR bug
# affects MetalBuffer cache-miss allocations and mutableCopy in the purge path.
# -fobjc-arc is scoped to this file only; the rest of the project uses MRR.
set_source_files_properties(
    ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
    PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
)

# ImGui static library — GLFW platform backend + Metal renderer backend (macOS)
# imgui_impl_metal.mm is Objective-C++; CMake handles .mm via OBJCXX language.
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
)

target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(imgui_lib PUBLIC
    # Silence Apple's OpenGL deprecation warnings (we don't use GL but some
    # system headers may pull it in)
    GL_SILENCE_DEPRECATION
)

target_link_libraries(imgui_lib PUBLIC
    glfw
    "-framework Metal"
    "-framework MetalKit"
    "-framework Foundation"
    "-framework QuartzCore"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# ── ImPlot ─────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(implot)

add_library(implot_lib STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp
)

target_include_directories(implot_lib PUBLIC ${implot_SOURCE_DIR})

# ImPlot must use the same imgui.h as imgui_lib (propagated via PUBLIC link)
target_link_libraries(implot_lib PUBLIC imgui_lib)

# ── spdlog ─────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)
# Use std::format (C++20 stdlib) instead of the bundled fmtlib.
# This avoids the FMT_STRING/consteval incompatibility with Apple Clang 21+.
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

# ── nlohmann-json ──────────────────────────────────────────────────────────────
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install    OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# ── Catch2 v3 ──────────────────────────────────────────────────────────────────
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

# ── HDF5 (system dependency: brew install hdf5) ────────────────────────────────
# Only the C API is used. The C API is stable across all 1.x releases so any
# brew-installed 1.8+ is compatible. No risk of version clashes.
find_package(HDF5 1.8 COMPONENTS C QUIET)
if(HDF5_FOUND)
    message(STATUS "HDF5 found: ${HDF5_VERSION} at ${HDF5_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR
        "HDF5 not found (need >= 1.8, C component).\n"
        "Install with:  brew install hdf5\n"
        "Then re-run:   cmake -B build"
    )
endif()
