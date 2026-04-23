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
)

target_link_libraries(gnc_viz_tests PRIVATE
    gnc_viz_lib
    Catch2::Catch2WithMain
)

gnc_viz_set_compile_options(gnc_viz_tests)

catch_discover_tests(gnc_viz_tests)
