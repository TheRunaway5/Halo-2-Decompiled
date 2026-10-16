# Complete source bodies and the real GPU consumer are available to their
# retained owners. Ordinary scheduling still requires its bitmap publication
# and parent/view owner; adding bodies does not synthesize a visible request.
target_sources(halo2_physics_world_runtime PRIVATE src/source_lens_draw_tu15_generated.cpp)
target_sources(halo2_renderer PRIVATE src/source_lens_draw_geometry_tu15.cpp src/source_lens_draw_gpu.cpp src/source_lens_draw_frame_tu15.cpp)
set(lens_draw_shaders)
foreach(shader source_lens_draw.vert source_lens_draw.frag)
    set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv")
    add_custom_command(OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND "${GLSLANG_VALIDATOR}" -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}" -o "${output}"
        DEPENDS "shaders/${shader}" shaders/source_lens_program.glsl shaders/combiner.glsl shaders/particle_depth.glsl VERBATIM)
    list(APPEND lens_draw_shaders "${output}")
endforeach()
add_custom_target(source_lens_draw_shaders DEPENDS ${lens_draw_shaders})
add_dependencies(halo2_renderer source_lens_draw_shaders)
if(BUILD_TESTING)
    add_executable(lens_draw_gpu_tests tests/lens_draw_gpu_tests.cpp)
    target_link_libraries(lens_draw_gpu_tests PRIVATE halo2_renderer halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_lens_draw_gpu COMMAND lens_draw_gpu_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_draw_geometry_tu15_hardware.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/headlong.map"
        "${CMAKE_CURRENT_BINARY_DIR}/lens-draw-gpu-validation"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_draw_callback_tu15_hardware.json")
    set_tests_properties(original_tu15_lens_draw_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(lens_draw_tu15_tests tests/lens_draw_tu15_tests.cpp)
    target_link_libraries(lens_draw_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_lens_draw_geometry COMMAND lens_draw_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_draw_geometry_tu15_hardware.json")
    set_tests_properties(original_tu15_lens_draw_geometry PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME original_tu15_lens_draw_callbacks COMMAND lens_draw_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_draw_callback_tu15_hardware.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/vehicle-motion-math-20261011/correspondence-verified/report.json")
    set_tests_properties(original_tu15_lens_draw_callbacks PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME original_tu15_lens_draw_setup COMMAND lens_draw_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_draw_setup_tu15_hardware.json")
    set_tests_properties(original_tu15_lens_draw_setup PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME original_tu15_lens_parent_gate COMMAND lens_draw_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_gate_tu15_hardware.json")
    set_tests_properties(original_tu15_lens_parent_gate PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
endif()
