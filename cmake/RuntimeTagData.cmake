include_guard(GLOBAL)

function(halo2_stage_tag_data target source)
    # The macOS engine bundle stages the complete data tree under Resources.
    # Contents/MacOS is a code directory and must not receive a duplicate copy.
    if(APPLE)
        return()
    endif()
    add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${source}" "$<TARGET_FILE_DIR:${target}>/data/tags")
endfunction()
