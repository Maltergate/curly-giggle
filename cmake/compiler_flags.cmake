# gnc_viz_set_compile_options(<target>)
#
# Apply strict warnings to our own targets (NOT to fetched dependencies).
# Call this on every gnc_viz_lib, gnc_viz, and gnc_viz_tests target.
function(gnc_viz_set_compile_options target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wno-deprecated-declarations   # Suppress Apple OpenGL / API deprecation noise
        -Wno-unused-parameter          # Relax during early scaffolding; tighten later
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${target} PRIVATE
            -g
            -fno-omit-frame-pointer
        )
        # AddressSanitizer + UBSan in Debug builds
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined)
        target_link_options(${target}    PRIVATE -fsanitize=address,undefined)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${target} PRIVATE -O3 -DNDEBUG)
    endif()
endfunction()
