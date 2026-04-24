add_library(fastscope_lib STATIC
    version.cpp
    error.cpp
    log.cpp
    ui_log_sink.cpp
    config.cpp
    signal_metadata.cpp
    signal_buffer.cpp
    signal_ops.cpp
    color_manager.cpp
    hdf5_reader.cpp
    simulation_file.cpp
    session.cpp
    operation_registry.cpp
    time_aligner.cpp
    axis_manager.cpp
    derived_signal.cpp
    csv_exporter.cpp
    png_exporter.cpp
)

target_include_directories(fastscope_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(fastscope_lib PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Link HDF5 C API only when available (will become REQUIRED once hdf5-reader is built)
if(HDF5_FOUND)
    target_include_directories(fastscope_lib PUBLIC ${HDF5_INCLUDE_DIRS})
    target_link_libraries(fastscope_lib PUBLIC ${HDF5_C_LIBRARIES})
    target_compile_definitions(fastscope_lib PUBLIC FASTSCOPE_HDF5_AVAILABLE)
endif()

fastscope_set_compile_options(fastscope_lib)
