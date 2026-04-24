add_executable(fastscope
    main.mm
    application.mm
    collapsible_pane.cpp
    file_open_dialog.mm
    timeseries_plot.cpp
    trajectory_2d_plot.cpp
    ground_track_plot.cpp
    plot_engine.cpp
    simulation_list_ui.cpp
    signal_tree_widget.cpp
    tool_manager.cpp
    annotation_tool.cpp
    ruler_tool.cpp
)

target_compile_definitions(fastscope PRIVATE
    GLFW_EXPOSE_NATIVE_COCOA   # needed for glfwGetCocoaWindow()
)

target_link_libraries(fastscope PRIVATE
    fastscope_lib
    imgui_lib
    implot_lib
    "-framework UniformTypeIdentifiers"
)

fastscope_set_compile_options(fastscope)

# Copy compile_commands.json to project root for clangd
add_custom_command(TARGET fastscope POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json to project root"
)

fastscope_set_compile_options(fastscope)

# Copy compile_commands.json to project root for clangd
add_custom_command(TARGET fastscope POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json to project root"
)
