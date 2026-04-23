add_library(gnc_viz_lib STATIC
    version.cpp
    error.cpp
)

target_include_directories(gnc_viz_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(gnc_viz_lib PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Link HDF5 C API only when available (will become REQUIRED once hdf5-reader is built)
if(HDF5_FOUND)
    target_include_directories(gnc_viz_lib PUBLIC ${HDF5_INCLUDE_DIRS})
    target_link_libraries(gnc_viz_lib PUBLIC ${HDF5_C_LIBRARIES})
    target_compile_definitions(gnc_viz_lib PUBLIC GNC_VIZ_HDF5_AVAILABLE)
endif()

gnc_viz_set_compile_options(gnc_viz_lib)
