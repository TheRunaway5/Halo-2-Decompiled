# Optional May 2003 renderer work stays separate from Original's TU1.5 owners.
add_library(halo2_pstencil STATIC src/pstencil/shadow.cpp)
target_sources(halo2_cache PRIVATE src/pstencil/material.cpp src/pstencil/environment.cpp)
target_sources(halo2_systems PRIVATE src/pstencil/point_lights.cpp src/pstencil/player_detail.cpp src/pstencil/point_caster.cpp)
target_include_directories(halo2_pstencil PUBLIC ${PROJECT_SOURCE_DIR}/include)
set_target_properties(halo2_pstencil PROPERTIES POSITION_INDEPENDENT_CODE ON)
foreach(program volume lightmap)
foreach(stage vert frag)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/${program}.${stage}.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/shaders/pstencil/${program}.${stage}
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/${program}.${stage}.spv
        DEPENDS shaders/pstencil/${program}.${stage} shaders/world_depth_fragment.glsl shaders/source_stipple.glsl VERBATIM)
endforeach()
endforeach()
add_custom_target(pstencil_shaders DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/volume.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/volume.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/lightmap.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/lightmap.frag.spv)
add_library(halo2_pstencil_render STATIC src/pstencil/geometry.cpp src/pstencil/volume.cpp src/pstencil/render.cpp src/pstencil/frame.cpp src/pstencil/lightmap.cpp)
target_link_libraries(halo2_pstencil_render PUBLIC halo2_pstencil halo2_cache Vulkan::Vulkan)
target_compile_definitions(halo2_pstencil_render PRIVATE HALO2_PSTENCIL_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_compile_options(halo2_pstencil_render PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers;-ffp-contract=off>)
add_dependencies(halo2_pstencil_render pstencil_shaders)
target_link_libraries(halo2_renderer PUBLIC halo2_pstencil_render)
set(PSTENCIL_2003_BUILD "" CACHE PATH "Local May 2003 Halo 2 build for original-code comparisons")
set(PSTENCIL_REFERENCE_PYTHON "${Python3_EXECUTABLE}" CACHE FILEPATH "Python with Unicorn for the optional pStencil original-code test")
if(BUILD_TESTING)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/tests/pstencil/environment.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv
        DEPENDS tests/pstencil/environment.comp shaders/combiner.glsl VERBATIM)
    add_custom_target(pstencil_environment_test_shader DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv)
    add_executable(pstencil_environment_tests tests/pstencil/environment_tests.cpp)
    target_link_libraries(pstencil_environment_tests PRIVATE halo2_cache Vulkan::Vulkan)
    add_dependencies(pstencil_environment_tests pstencil_environment_test_shader)
    add_test(NAME pstencil_environment_response COMMAND pstencil_environment_tests
        ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/fallback-environment.json
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv
        ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/default_environment_map-resident-0.bin)
    add_executable(pstencil_audit_caster_resources tools/pstencil/audit_caster_resources.cpp)
    target_link_libraries(pstencil_audit_caster_resources PRIVATE halo2_pstencil_render)
    add_executable(pstencil_caster_tests tests/pstencil/caster_tests.cpp)
    target_link_libraries(pstencil_caster_tests PRIVATE halo2_pstencil_render)
    add_test(NAME pstencil_caster_faces COMMAND pstencil_caster_tests ${PROJECT_SOURCE_DIR}/assets/disc/maps/zanzibar.map)
    add_executable(pstencil_gameplay_capture tools/pstencil/capture_gameplay.cpp src/multiplayer_runtime.cpp)
    target_link_libraries(pstencil_gameplay_capture PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan glm::glm)
    target_compile_definitions(pstencil_gameplay_capture PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_dependencies(pstencil_gameplay_capture halo2_shaders)
    add_executable(pstencil_point_lights_tests tests/pstencil/point_lights_tests.cpp)
    target_link_libraries(pstencil_point_lights_tests PRIVATE halo2_systems)
    add_test(NAME pstencil_plasma_point_light COMMAND pstencil_point_lights_tests ${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    add_executable(pstencil_armor_scene tools/pstencil/capture_armor_scene.cpp)
    target_link_libraries(pstencil_armor_scene PRIVATE halo2_renderer)
    add_executable(pstencil_dynamic_materials tools/pstencil/capture_dynamic_materials.cpp)
    target_link_libraries(pstencil_dynamic_materials PRIVATE halo2_renderer)
    add_library(pstencil_original_bridge SHARED tests/pstencil/original_bridge.cpp)
    target_link_libraries(pstencil_original_bridge PRIVATE halo2_pstencil)
    add_executable(pstencil_geometry_tests tests/pstencil/geometry_tests.cpp)
    target_link_libraries(pstencil_geometry_tests PRIVATE halo2_pstencil_render)
    add_executable(pstencil_renderer_tests tests/pstencil/renderer_tests.cpp)
    target_link_libraries(pstencil_renderer_tests PRIVATE halo2_renderer)
    target_compile_definitions(pstencil_renderer_tests PRIVATE HALO2_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME pstencil_real_map_roundtrip COMMAND ${CMAKE_COMMAND}
        -DTEST_BINARY=$<TARGET_FILE:pstencil_renderer_tests>
        -DFIXTURES=${PROJECT_SOURCE_DIR}/analysis/multiplayer-shading/world-effects-goal-20261011/light-raw-bindings-oracle-v2
        -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/pstencil/renderer-results
        -P ${PROJECT_SOURCE_DIR}/tests/pstencil/run_renderer.cmake)
    add_executable(pstencil_ui_tests tests/pstencil/ui_tests.cpp)
    target_link_libraries(pstencil_ui_tests PRIVATE halo2_renderer halo2_imgui)
    target_compile_definitions(pstencil_ui_tests PRIVATE HALO2_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME pstencil_live_graphics_selector COMMAND pstencil_ui_tests
        ${CMAKE_CURRENT_BINARY_DIR}/pstencil/ui-results)
    add_test(NAME pstencil_original_world_positions COMMAND pstencil_geometry_tests
        ${PROJECT_SOURCE_DIR}/assets/disc/maps/zanzibar.map
        ${PROJECT_SOURCE_DIR}/tests/fixtures/retail_shadow_caster_world.tsv)
endif()
if(BUILD_TESTING AND PSTENCIL_2003_BUILD AND EXISTS "${PSTENCIL_2003_BUILD}/game/halo2debug.xbe")
    add_test(NAME pstencil_original_shadow
        COMMAND ${PSTENCIL_REFERENCE_PYTHON} ${PROJECT_SOURCE_DIR}/tools/pstencil/verify_original.py
            --build ${PSTENCIL_2003_BUILD} --native $<TARGET_FILE:pstencil_original_bridge>
            --output ${CMAKE_CURRENT_BINARY_DIR}/pstencil/original-shadow.json)
endif()
