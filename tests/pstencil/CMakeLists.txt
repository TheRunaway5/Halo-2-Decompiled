cmake_minimum_required(VERSION 3.24)
project(PStencilGPU LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED CONFIG)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)
get_filename_component(ROOT ../.. ABSOLUTE)
foreach(shader volume.vert volume.geom volume.frag lightmap.vert lightmap.frag)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/${shader}.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${ROOT}/shaders ${ROOT}/shaders/pstencil/${shader}
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/${shader}.spv
        DEPENDS ${ROOT}/shaders/pstencil/${shader} ${ROOT}/shaders/world_depth_fragment.glsl ${ROOT}/shaders/source_stipple.glsl VERBATIM)
endforeach()
foreach(stage vert geom frag)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/fixture.${stage}.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -I${ROOT}/shaders -I${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/fixture.${stage}
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/fixture.${stage}.spv
        DEPENDS fixture.${stage} captured_surface.glsl ${ROOT}/shaders/vertex_program.glsl ${ROOT}/shaders/render_view.glsl ${ROOT}/shaders/world_depth_fragment.glsl VERBATIM)
endforeach()
add_custom_target(shaders DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/volume.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/volume.geom.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/volume.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/lightmap.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/lightmap.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/fixture.vert.spv ${CMAKE_CURRENT_BINARY_DIR}/shaders/fixture.geom.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/fixture.frag.spv)
add_executable(pstencil_gpu gpu_tests.cpp ${ROOT}/src/pstencil/shadow.cpp ${ROOT}/src/pstencil/volume.cpp ${ROOT}/src/pstencil/render.cpp ${ROOT}/src/pstencil/lightmap.cpp)
target_include_directories(pstencil_gpu PRIVATE ${ROOT}/include)
target_link_libraries(pstencil_gpu PRIVATE Vulkan::Vulkan glm::glm)
target_compile_definitions(pstencil_gpu PRIVATE HALO2_PSTENCIL_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_compile_options(pstencil_gpu PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -ffp-contract=off)
# This component owns volume submission; unused sun-query entry points in
# lightmap.cpp have whole-world dependencies and are not fixture inputs.
target_compile_options(pstencil_gpu PRIVATE -ffunction-sections -fdata-sections)
target_link_options(pstencil_gpu PRIVATE -Wl,--gc-sections)
add_dependencies(pstencil_gpu shaders)
add_test(NAME pstencil_volume_depth_gpu COMMAND ${Python3_EXECUTABLE} ${ROOT}/tools/run_test.py
    --scratch-root ${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary/scratch --
    $<TARGET_FILE:pstencil_gpu> @HALO2_TEST_SCRATCH@/evidence)
set_tests_properties(pstencil_volume_depth_gpu PROPERTIES TIMEOUT 180 ENVIRONMENT "VK_LAYER_VALIDATE_SYNC=1")
