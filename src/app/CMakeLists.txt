add_executable(gnc_viz main.cpp)

target_link_libraries(gnc_viz PRIVATE
    gnc_viz_lib
    imgui_lib
    implot_lib
)

gnc_viz_set_compile_options(gnc_viz)

# Copy compile_commands.json to project root for clangd
add_custom_command(TARGET gnc_viz POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json to project root"
)
