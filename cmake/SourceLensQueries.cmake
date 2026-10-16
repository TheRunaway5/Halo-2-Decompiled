target_sources(halo2_physics_world_runtime PRIVATE src/source_lens_query_tu15_generated.cpp)
target_sources(halo2_renderer PRIVATE src/source_lens_query_gpu.cpp src/source_lens_query_geometry_tu15.cpp src/source_lens_query_frame_tu15.cpp)
set(lens_query_shaders)
foreach(shader source_lens_query.vert source_lens_query.frag)
    set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv")
    add_custom_command(OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND "${GLSLANG_VALIDATOR}" -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}" -o "${output}"
        DEPENDS "shaders/${shader}" shaders/particle_depth.glsl VERBATIM)
    list(APPEND lens_query_shaders "${output}")
endforeach()
add_custom_target(source_lens_query_shaders DEPENDS ${lens_query_shaders})
add_dependencies(halo2_renderer source_lens_query_shaders)

if(BUILD_TESTING)
    add_executable(lens_query_tu15_tests tests/lens_query_tu15_tests.cpp)
    target_link_libraries(lens_query_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_lens_query_quads COMMAND lens_query_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_quad_tu15_hardware.json")
    set_tests_properties(original_tu15_lens_query_quads PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_test(NAME original_tu15_lens_query_state COMMAND lens_query_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_query_state_tu15_hardware.json")
    set_tests_properties(original_tu15_lens_query_state PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_test(NAME original_tu15_lens_query_reuse COMMAND lens_query_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_query_reuse_tu15_hardware.json")
    set_tests_properties(original_tu15_lens_query_reuse PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_executable(lens_query_gpu_tests tests/lens_query_gpu_tests.cpp)
    target_link_libraries(lens_query_gpu_tests PRIVATE halo2_renderer halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME source_lens_query_gpu COMMAND lens_query_gpu_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_quad_tu15_hardware.json"
        "${CMAKE_CURRENT_BINARY_DIR}/lens-query-gpu-validation")
    set_tests_properties(source_lens_query_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
