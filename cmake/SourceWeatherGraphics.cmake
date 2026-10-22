# Explicit-input weather passes. The live renderer must supply independently
# admitted current producers before it can call these modules.
function(halo2_add_source_weather_graphics weather_source_root)
    set(weather_shader_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(weather_shader_outputs)
    foreach(weather_shader source_weather_point.vert source_weather_point.frag
                           source_weather_line.vert source_weather_line.geom source_weather_line.frag
                           source_weather_plate.vert source_weather_plate.frag)
        set(weather_output "${weather_shader_directory}/${weather_shader}.spv")
        add_custom_command(OUTPUT "${weather_output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${weather_shader_directory}"
            COMMAND "${GLSLANG_VALIDATOR}" -V --target-env vulkan1.1
                "-I${weather_source_root}/shaders"
                "${weather_source_root}/shaders/${weather_shader}" -o "${weather_output}"
            DEPENDS "${weather_source_root}/shaders/${weather_shader}"
                "${weather_source_root}/shaders/source_weather_point_program.glsl"
                "${weather_source_root}/shaders/vertex_program.glsl"
                "${weather_source_root}/shaders/combiner.glsl"
                "${weather_source_root}/shaders/particle_depth.glsl"
            VERBATIM)
        list(APPEND weather_shader_outputs "${weather_output}")
    endforeach()
    add_custom_target(source_weather_graphics_shaders DEPENDS ${weather_shader_outputs})
    add_library(halo2_weather_render
        "${weather_source_root}/src/rendering/weather/source_weather_point_render.cpp"
        "${weather_source_root}/src/rendering/weather/source_weather_line_render.cpp"
        "${weather_source_root}/src/rendering/weather/source_weather_plate_render.cpp"
        "${weather_source_root}/src/rendering/weather/source_weather_plate_caller_render.cpp")
    target_include_directories(halo2_weather_render PUBLIC "${weather_source_root}/include")
    # Typed preparation derives its own complete caller program/projection.
    # The legacy renderer translation unit keeps no builder dependency, so
    # existing standalone captured-packet scripts still link unchanged.
    target_link_libraries(halo2_weather_render PUBLIC
        halo2_cache Vulkan::Vulkan halo2_weather_simulation)
    add_dependencies(halo2_weather_render source_weather_graphics_shaders)
    if(BUILD_TESTING AND EXISTS "${weather_source_root}/analysis/multiplayer-shading/weather-particle-packet/source-vs22-class0.vp")
        add_executable(source_weather_line_render_tests "${weather_source_root}/tests/source_weather_line_render_tests.cpp")
        target_link_libraries(source_weather_line_render_tests PRIVATE halo2_weather_render)
        target_compile_definitions(source_weather_line_render_tests PRIVATE
            HALO2_SOURCE_DIR="${weather_source_root}"
            HALO2_WEATHER_POINT_SHADER_DIR="${weather_shader_directory}")
        add_test(NAME source_weather_line_render_gpu COMMAND source_weather_line_render_tests)
        set_tests_properties(source_weather_line_render_gpu PROPERTIES SKIP_RETURN_CODE 77 LABELS "weather;gpu")
    endif()
    if(MSVC)
        target_compile_options(halo2_weather_render PRIVATE /fp:strict)
    else()
        target_compile_options(halo2_weather_render PRIVATE -fno-fast-math -ffp-contract=off)
    endif()

    # Actual original snapshots are private analysis artifacts, not synthetic
    # repository fixtures. Opt in explicitly when the retained banks exist.
    option(HALO2_WEATHER_REFERENCE_TESTS "Build tests using retained original weather captures" OFF)
    if(NOT BUILD_TESTING OR NOT HALO2_WEATHER_REFERENCE_TESTS)
        return()
    endif()
    foreach(weather_required
        "analysis/multiplayer-shading/weather-particle-packet/emitted-source-v0-logical-color.bin"
        "analysis/multiplayer-shading/weather-point-prototype/vertex-analytic.bin"
        "analysis/multiplayer-shading/weather-point-original-reference-event215873/points-v0-logical-argb.bin"
        "analysis/multiplayer-shading/weather-plate-original-reference/event-215778/source-mode3-pixel-definition.bin")
        if(NOT EXISTS "${weather_source_root}/${weather_required}")
            message(FATAL_ERROR "Retained original weather reference missing: ${weather_required}")
        endif()
    endforeach()
    set(weather_probe "${weather_shader_directory}/source_weather_point_probe.comp.spv")
    add_custom_command(OUTPUT "${weather_probe}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${weather_shader_directory}"
        COMMAND "${GLSLANG_VALIDATOR}" -V --target-env vulkan1.1
            "-I${weather_source_root}/shaders"
            "${weather_source_root}/shaders/source_weather_point_probe.comp" -o "${weather_probe}"
        DEPENDS "${weather_source_root}/shaders/source_weather_point_probe.comp"
            "${weather_source_root}/shaders/source_weather_point_program.glsl"
            "${weather_source_root}/shaders/vertex_program.glsl"
        VERBATIM)
    add_custom_target(source_weather_point_probe_shader DEPENDS "${weather_probe}")
    foreach(weather_test source_weather_point_render source_weather_point_scene source_weather_plate_scene)
        add_executable(${weather_test}_tests "${weather_source_root}/tests/${weather_test}_tests.cpp")
        target_link_libraries(${weather_test}_tests PRIVATE halo2_weather_render)
        target_include_directories(${weather_test}_tests PRIVATE "${weather_source_root}/tests")
        target_compile_definitions(${weather_test}_tests PRIVATE
            HALO2_SOURCE_DIR="${weather_source_root}"
            HALO2_WEATHER_POINT_SHADER_DIR="${weather_shader_directory}"
            HALO2_WEATHER_PLATE_SHADER_DIR="${weather_shader_directory}"
            HALO2_WEATHER_POINT_PROBE_SHADER="${weather_probe}")
        if(MSVC)
            target_compile_options(${weather_test}_tests PRIVATE /fp:strict)
        else()
            target_compile_options(${weather_test}_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_${weather_test}_cpu COMMAND ${weather_test}_tests)
        add_test(NAME original_${weather_test}_gpu COMMAND ${weather_test}_tests --gpu)
        # Tests save their readbacks in the reference's analysis directory.
        # Serialize them to preserve a single coherent readback set.
        set_tests_properties(original_${weather_test}_cpu original_${weather_test}_gpu
            PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE LABELS "weather;original-reference")
    endforeach()
    add_dependencies(source_weather_point_render_tests source_weather_point_probe_shader)
endfunction()
