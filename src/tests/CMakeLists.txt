include(CTest)
include(Catch)

add_executable(gnc_viz_tests
    test_infra.cpp
    test_error.cpp
    test_log.cpp
    test_app_state.cpp
    test_config.cpp
    test_interfaces.cpp
    test_registry.cpp
    test_signal_metadata.cpp
    test_signal_buffer.cpp
    test_color_manager.cpp
    test_signal_ops.cpp
    test_hdf5_reader.cpp
    test_time_aligner.cpp
    test_axis_manager.cpp
    test_derived_signal.cpp
    test_tool_manager.cpp
    test_session.cpp
    test_csv_exporter.cpp
    test_system.cpp
    # ToolManager sources (depend on ImGui/ImPlot but no context needed for registry tests)
    ../app/tool_manager.cpp
    ../app/annotation_tool.cpp
    ../app/ruler_tool.cpp
)

target_link_libraries(gnc_viz_tests PRIVATE
    gnc_viz_lib
    imgui_lib
    implot_lib
    Catch2::Catch2WithMain
)

gnc_viz_set_compile_options(gnc_viz_tests)

catch_discover_tests(gnc_viz_tests)
