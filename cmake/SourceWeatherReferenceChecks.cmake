# Connected captured-reference validation, with no new live renderer entry.
function(halo2_add_source_weather_reference_checks weather_source_root)
    if(NOT BUILD_TESTING OR NOT HALO2_WEATHER_REFERENCE_TESTS)
        return()
    endif()
    set(weather_shader_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_executable(source_weather_connected_scene_tests
        "${weather_source_root}/tests/source_weather_connected_scene_tests.cpp")
    target_include_directories(source_weather_connected_scene_tests PRIVATE "${weather_source_root}/tests")
    target_link_libraries(source_weather_connected_scene_tests PRIVATE
        halo2_weather_depth_alias halo2_weather_render)
    target_compile_definitions(source_weather_connected_scene_tests PRIVATE
        HALO2_SOURCE_DIR="${weather_source_root}"
        HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR="${weather_shader_directory}"
        HALO2_WEATHER_PLATE_SHADER_DIR="${weather_shader_directory}"
        HALO2_WEATHER_POINT_SHADER_DIR="${weather_shader_directory}")
    if(MSVC)
        target_compile_options(source_weather_connected_scene_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_weather_connected_scene_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME original_source_weather_connected_scene_cpu COMMAND source_weather_connected_scene_tests)
    add_test(NAME original_source_weather_connected_scene_gpu COMMAND source_weather_connected_scene_tests --gpu)
    set_tests_properties(original_source_weather_connected_scene_cpu original_source_weather_connected_scene_gpu
        PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE LABELS "weather;original-reference;connected")
    add_test(NAME original_source_weather_connected_scene_gpu_exact
        COMMAND source_weather_connected_scene_tests --gpu-exact)
    set_tests_properties(original_source_weather_connected_scene_gpu_exact PROPERTIES
        SKIP_RETURN_CODE 77 RUN_SERIAL TRUE
        LABELS "weather;original-reference;connected;exact-pixels")

    # The frozen source arithmetic helper currently admits x86 SSE host
    # controls only. Do not register typed fixture/GPU checks on other hosts.
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64|i[3-6]86)$")
        return()
    endif()
    foreach(weather_required
        "tests/fixtures/retail_source_weather_plate_caller.tsv"
        "tests/fixtures/retail_source_weather_plate_program.tsv"
        "analysis/multiplayer-shading/weather-plate-typed-prepare-review/actual-source-caller-input21.bin"
        "analysis/multiplayer-shading/weather-plate-scene-prototype/native-after-scene-color.rgba")
        if(NOT EXISTS "${weather_source_root}/${weather_required}")
            message(FATAL_ERROR "Retained typed weather reference missing: ${weather_required}")
        endif()
    endforeach()
    set(weather_typed_probe "${weather_shader_directory}/source_weather_plate_combiner_probe.comp.spv")
    add_custom_command(OUTPUT "${weather_typed_probe}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${weather_shader_directory}"
        COMMAND "${GLSLANG_VALIDATOR}" -V --target-env vulkan1.1
            "-I${weather_source_root}/shaders"
            "${weather_source_root}/tests/source_weather_plate_combiner_probe.comp" -o "${weather_typed_probe}"
        DEPENDS "${weather_source_root}/tests/source_weather_plate_combiner_probe.comp"
            "${weather_source_root}/shaders/combiner.glsl"
        VERBATIM)
    add_custom_target(source_weather_plate_combiner_probe_shader DEPENDS "${weather_typed_probe}")
    add_executable(source_weather_plate_typed_tests
        "${weather_source_root}/tests/source_weather_plate_typed_tests.cpp")
    target_include_directories(source_weather_plate_typed_tests PRIVATE "${weather_source_root}/tests")
    target_link_libraries(source_weather_plate_typed_tests PRIVATE halo2_weather_render)
    target_compile_definitions(source_weather_plate_typed_tests PRIVATE
        HALO2_SOURCE_DIR="${weather_source_root}"
        HALO2_WEATHER_PLATE_SHADER_DIR="${weather_shader_directory}")
    if(MSVC)
        target_compile_options(source_weather_plate_typed_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_weather_plate_typed_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_dependencies(source_weather_plate_typed_tests source_weather_plate_combiner_probe_shader)
    add_test(NAME original_source_weather_plate_typed_cpu
        COMMAND source_weather_plate_typed_tests --cpu)
    add_test(NAME original_source_weather_plate_typed_scene_gpu
        COMMAND source_weather_plate_typed_tests --gpu-scene
            "${CMAKE_CURRENT_BINARY_DIR}/weather-typed-reference/scene")
    add_test(NAME source_weather_plate_typed_combiner_gpu
        COMMAND source_weather_plate_typed_tests --gpu-combiner
            "${CMAKE_CURRENT_BINARY_DIR}/weather-typed-reference/combiner")
    set_tests_properties(original_source_weather_plate_typed_cpu original_source_weather_plate_typed_scene_gpu
        PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE LABELS "weather;original-reference;typed")
    # Native combiner/scalar arithmetic over original definitions is distinct
    # from the retained actual scene's original raster comparison.
    set_tests_properties(source_weather_plate_typed_combiner_gpu
        PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE LABELS "weather;source-fixture;typed;combiner")
endfunction()
