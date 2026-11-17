# The moving native triangles and raster-jitter regression must run against
# the production shader outputs that the launcher package will install.
set(pstencil_depth_fixture_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil-depth-fixture")
set(pstencil_depth_fixture_shaders)
foreach(stage vert geom frag)
    set(output "${pstencil_depth_fixture_directory}/fixture.${stage}.spv")
    add_custom_command(OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${pstencil_depth_fixture_directory}"
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            -I${PROJECT_SOURCE_DIR}/tests/pstencil
            ${PROJECT_SOURCE_DIR}/tests/pstencil/fixture.${stage} -o "${output}"
        DEPENDS tests/pstencil/fixture.${stage} tests/pstencil/captured_surface.glsl
            shaders/vertex_program.glsl shaders/render_view.glsl shaders/world_depth_fragment.glsl
        VERBATIM)
    list(APPEND pstencil_depth_fixture_shaders "${output}")
endforeach()
add_custom_target(pstencil_depth_fixture_shaders DEPENDS ${pstencil_depth_fixture_shaders})
add_executable(pstencil_volume_depth_gpu_tests tests/pstencil/gpu_tests.cpp
    src/pstencil/shadow.cpp src/pstencil/volume.cpp src/pstencil/render.cpp src/pstencil/lightmap.cpp)
target_include_directories(pstencil_volume_depth_gpu_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(pstencil_volume_depth_gpu_tests PRIVATE Vulkan::Vulkan halo2_glm)
target_compile_definitions(pstencil_volume_depth_gpu_tests PRIVATE
    HALO2_PSTENCIL_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders"
    HALO2_PSTENCIL_FIXTURE_SHADER_DIR="${pstencil_depth_fixture_directory}")
# The volume fixture does not call the sun-query helpers whose world owners
# are in lightmap.cpp. Keep this a small, independently runnable GPU target.
target_compile_options(pstencil_volume_depth_gpu_tests PRIVATE
    -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -ffp-contract=off
    -ffunction-sections -fdata-sections)
if(APPLE)
    target_link_options(pstencil_volume_depth_gpu_tests PRIVATE -Wl,-dead_strip)
else()
    target_link_options(pstencil_volume_depth_gpu_tests PRIVATE -Wl,--gc-sections)
endif()
add_dependencies(pstencil_volume_depth_gpu_tests pstencil_shaders pstencil_depth_fixture_shaders)
add_test(NAME pstencil_volume_depth_gpu COMMAND pstencil_volume_depth_gpu_tests
    @HALO2_TEST_SCRATCH@/evidence)
set_tests_properties(pstencil_volume_depth_gpu PROPERTIES TIMEOUT 180 ENVIRONMENT "VK_LAYER_VALIDATE_SYNC=1")
