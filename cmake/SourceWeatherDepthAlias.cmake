# Exact integer producer for a caller-owned current D24S8 image. Kept separate
# from graphics and simulation so no native frame admission is implied.
function(halo2_add_source_weather_depth_alias weather_source_root)
    set(weather_shader_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(weather_alias_shader "${weather_shader_directory}/source_weather_depth_alias.comp.spv")
    add_custom_command(OUTPUT "${weather_alias_shader}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${weather_shader_directory}"
        COMMAND "${GLSLANG_VALIDATOR}" -V --target-env vulkan1.1
            "${weather_source_root}/shaders/source_weather_depth_alias.comp" -o "${weather_alias_shader}"
        DEPENDS "${weather_source_root}/shaders/source_weather_depth_alias.comp"
        VERBATIM)
    add_custom_target(source_weather_depth_alias_shader DEPENDS "${weather_alias_shader}")
    add_library(halo2_weather_depth_alias "${weather_source_root}/src/source_weather_depth_alias.cpp")
    target_include_directories(halo2_weather_depth_alias PUBLIC "${weather_source_root}/include")
    target_link_libraries(halo2_weather_depth_alias PUBLIC Vulkan::Vulkan)
    add_dependencies(halo2_weather_depth_alias source_weather_depth_alias_shader)
    if(BUILD_TESTING)
        add_executable(source_weather_depth_alias_frame_tests "${weather_source_root}/tests/source_weather_depth_alias_frame_tests.cpp")
        target_link_libraries(source_weather_depth_alias_frame_tests PRIVATE halo2_weather_depth_alias)
        target_compile_definitions(source_weather_depth_alias_frame_tests PRIVATE
            HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR="${weather_shader_directory}")
        add_test(NAME source_weather_depth_alias_frame_gpu COMMAND source_weather_depth_alias_frame_tests)
        set_tests_properties(source_weather_depth_alias_frame_gpu PROPERTIES SKIP_RETURN_CODE 77 LABELS "weather;gpu")
    endif()
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(halo2_weather_depth_alias PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(halo2_weather_depth_alias PUBLIC -fsanitize=address,undefined)
    endif()

    if(NOT BUILD_TESTING OR NOT HALO2_WEATHER_REFERENCE_TESTS)
        return()
    endif()
    foreach(weather_required before-scene-depth.raw stage-0-mip-0.rgba)
        if(NOT EXISTS "${weather_source_root}/analysis/multiplayer-shading/weather-plate-original-reference/event-215778/${weather_required}")
            message(FATAL_ERROR "Retained original depth reference missing: ${weather_required}")
        endif()
    endforeach()
    set(weather_alias_probe "${weather_shader_directory}/source_weather_depth_alias_sample.comp.spv")
    add_custom_command(OUTPUT "${weather_alias_probe}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${weather_shader_directory}"
        COMMAND "${GLSLANG_VALIDATOR}" -V --target-env vulkan1.1
            "${weather_source_root}/tests/source_weather_depth_alias_sample.comp" -o "${weather_alias_probe}"
        DEPENDS "${weather_source_root}/tests/source_weather_depth_alias_sample.comp"
        VERBATIM)
    add_custom_target(source_weather_depth_alias_probe_shader DEPENDS "${weather_alias_probe}")
    foreach(weather_test source_weather_depth_alias source_weather_depth_alias_composition)
        add_executable(${weather_test}_tests "${weather_source_root}/tests/${weather_test}_tests.cpp")
        target_include_directories(${weather_test}_tests PRIVATE "${weather_source_root}/tests")
        target_link_libraries(${weather_test}_tests PRIVATE halo2_weather_depth_alias)
        target_compile_definitions(${weather_test}_tests PRIVATE
            HALO2_SOURCE_DIR="${weather_source_root}"
            HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR="${weather_shader_directory}"
            HALO2_WEATHER_PLATE_SHADER_DIR="${weather_shader_directory}")
        if(MSVC)
            target_compile_options(${weather_test}_tests PRIVATE /fp:strict)
        else()
            target_compile_options(${weather_test}_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_${weather_test}_cpu COMMAND ${weather_test}_tests)
        add_test(NAME original_${weather_test}_gpu COMMAND ${weather_test}_tests --gpu)
        set_tests_properties(original_${weather_test}_cpu original_${weather_test}_gpu
            PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE LABELS "weather;original-reference")
    endforeach()
    add_dependencies(source_weather_depth_alias_tests source_weather_depth_alias_probe_shader)
    target_link_libraries(source_weather_depth_alias_composition_tests PRIVATE halo2_weather_render)
endfunction()
