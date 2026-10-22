# Optional May 2003 renderer work stays separate from Original's TU1.5 owners.
add_library(halo2_pstencil STATIC src/pstencil/shadow.cpp)
target_sources(halo2_systems PRIVATE src/pstencil/native_scenario_lights_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/pstencil/campaign_renderer.cpp)
target_sources(halo2_cache PRIVATE src/pstencil/combined_environment.cpp)
target_sources(halo2_pstencil PRIVATE src/pstencil/light_response_2003.cpp src/pstencil/texture_accumulation_2003.cpp)
target_sources(halo2_pstencil PRIVATE src/pstencil/reference_light_materials.cpp)
target_sources(halo2_pstencil PRIVATE src/pstencil/projected_constants_2003.cpp)
target_link_libraries(halo2_cache PRIVATE halo2_pstencil)
target_sources(halo2_renderer PRIVATE src/pstencil/campaign_materials.cpp)
target_sources(halo2_cache PRIVATE src/pstencil/material.cpp src/pstencil/environment.cpp src/pstencil/detail.cpp src/pstencil/specular_mask.cpp src/pstencil/projected_light_material.cpp)
target_sources(halo2_systems PRIVATE src/pstencil/point_lights.cpp src/pstencil/attached_lights.cpp src/pstencil/native_light_update_tu15_generated.cpp src/pstencil/shared_light_materials.cpp src/pstencil/native_incident.cpp src/pstencil/native_lights.cpp src/pstencil/native_light_shape.cpp src/pstencil/native_light_shape_tu15_generated.cpp src/pstencil/native_light_constants.cpp src/pstencil/native_light_receiver.cpp src/pstencil/projected_light_selection.cpp src/pstencil/projected_lights.cpp src/pstencil/native_light_portal_tu15_generated.cpp src/pstencil/native_light_constants_tu15_generated.cpp src/pstencil/campaign.cpp src/pstencil/campaign_lights.cpp src/pstencil/player_detail.cpp src/pstencil/point_caster.cpp)
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
foreach(program light_utility dependent_light normal_accumulation normal_light_utility texture_accumulation)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/${program}.frag.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/shaders/pstencil/${program}.frag
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/${program}.frag.spv
        DEPENDS shaders/pstencil/${program}.frag shaders/pstencil/light_surface.glsl
            shaders/combiner.glsl shaders/texture_program.glsl shaders/world_combiner_specialization.glsl
            shaders/world_depth_fragment.glsl shaders/source_stipple.glsl
            shaders/pstencil/texture_accumulation_2003_program.glsl VERBATIM)
endforeach()
foreach(program normal_accumulation normal_light_utility)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/${program}.vert.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/shaders/pstencil/${program}.vert
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/${program}.vert.spv
        DEPENDS shaders/pstencil/${program}.vert shaders/world_vertex.glsl shaders/vertex_program.glsl
            shaders/source_cpu_skin_outputs.glsl VERBATIM)
endforeach()
add_custom_target(pstencil_shaders DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/texture_accumulation.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/normal_accumulation.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/normal_accumulation.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/normal_light_utility.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/normal_light_utility.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/volume.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/volume.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/lightmap.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/lightmap.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/light_utility.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/dependent_light.frag.spv)
add_executable(pstencil_projected_light_tests tests/pstencil/projected_light_material_tests.cpp)
target_link_libraries(pstencil_projected_light_tests PRIVATE halo2_cache)
add_test(NAME pstencil_projected_light_selection COMMAND pstencil_projected_light_tests
    ${PROJECT_SOURCE_DIR}/tests/pstencil/fixtures/projected_light_selection_tu15.tsv
    ${PROJECT_SOURCE_DIR}/tests/pstencil/fixtures/projected_light_features_tu15.tsv)
add_library(halo2_pstencil_render STATIC src/pstencil/geometry.cpp src/pstencil/volume.cpp src/pstencil/render.cpp src/pstencil/frame.cpp src/pstencil/lightmap.cpp)
target_link_libraries(halo2_pstencil_render PUBLIC halo2_pstencil halo2_cache Vulkan::Vulkan)
target_compile_definitions(halo2_pstencil_render PRIVATE HALO2_PSTENCIL_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_compile_options(halo2_pstencil_render PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers;-ffp-contract=off>)
add_dependencies(halo2_pstencil_render pstencil_shaders)
target_link_libraries(halo2_renderer PUBLIC halo2_pstencil_render)
set(PSTENCIL_2003_BUILD "" CACHE PATH "Local May 2003 Halo 2 build for original-code comparisons")
set(PSTENCIL_REFERENCE_PYTHON "${Python3_EXECUTABLE}" CACHE FILEPATH "Python with Unicorn for the optional pStencil original-code test")
if(BUILD_TESTING)
    add_executable(pstencil_texture_accumulation_context_tests tests/pstencil/texture_accumulation_context_tests.cpp src/pstencil/detail.cpp)
    target_link_libraries(pstencil_texture_accumulation_context_tests PRIVATE halo2_pstencil)
    if(EXISTS "${PROJECT_SOURCE_DIR}/analysis/pstencil/texture-accumulation-inputs-v1/current-cairo/inputs.json")
        add_test(NAME pstencil_texture_accumulation_context COMMAND pstencil_texture_accumulation_context_tests
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/texture-accumulation-inputs-v1/current-cairo/inputs.json)
    endif()
    add_executable(pstencil_texture_accumulation_gpu_tests tests/pstencil/texture_accumulation_gpu_tests.cpp)
    target_link_libraries(pstencil_texture_accumulation_gpu_tests PRIVATE halo2_pstencil Vulkan::Vulkan)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/texture_accumulation_2003.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/tests/pstencil/texture_accumulation_2003.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/texture_accumulation_2003.comp.spv
        DEPENDS tests/pstencil/texture_accumulation_2003.comp shaders/combiner.glsl
            shaders/pstencil/texture_accumulation_2003_program.glsl VERBATIM)
    add_custom_target(pstencil_texture_accumulation_gpu_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/texture_accumulation_2003.comp.spv)
    add_dependencies(pstencil_texture_accumulation_gpu_tests pstencil_texture_accumulation_gpu_shader)
    if(EXISTS "${PROJECT_SOURCE_DIR}/analysis/pstencil/texture-accumulation-inputs-v1/current-cairo/inputs.json")
        add_test(NAME pstencil_texture_accumulation_gpu_composition COMMAND pstencil_texture_accumulation_gpu_tests
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/texture-accumulation-inputs-v1/current-cairo/inputs.json
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/texture_accumulation_2003.comp.spv)
        set_tests_properties(pstencil_texture_accumulation_gpu_composition PROPERTIES RUN_SERIAL TRUE)
    endif()
    add_executable(pstencil_texture_accumulation_routing_tests tests/pstencil/texture_accumulation_routing_tests.cpp src/pstencil/specular_mask.cpp)
    target_link_libraries(pstencil_texture_accumulation_routing_tests PRIVATE halo2_pstencil)
    add_test(NAME pstencil_texture_accumulation_routing COMMAND pstencil_texture_accumulation_routing_tests)
    add_executable(pstencil_texture_accumulation_2003_tests tests/pstencil/texture_accumulation_2003_tests.cpp)
    target_link_libraries(pstencil_texture_accumulation_2003_tests PRIVATE halo2_pstencil)
    if(EXISTS "${PROJECT_SOURCE_DIR}/analysis/pstencil/texture-accumulation-inputs-v1/current-cairo/inputs.json")
        add_test(NAME pstencil_original_texture_accumulation COMMAND pstencil_texture_accumulation_2003_tests
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/accumulation-construction-v1/reference.json
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/texture-accumulation-inputs-v1/current-cairo/inputs.json)
    endif()
    add_executable(pstencil_normal_accumulation_2003_tests tests/pstencil/normal_accumulation_2003_tests.cpp)
    target_link_libraries(pstencil_normal_accumulation_2003_tests PRIVATE halo2_pstencil)
    if(EXISTS "${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/accumulation-bindings-v1/reference.json")
        add_test(NAME pstencil_original_normal_accumulation COMMAND pstencil_normal_accumulation_2003_tests
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/accumulation-construction-v1/reference.json
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/accumulation-bindings-v1/reference.json)
    endif()
    add_executable(pstencil_light_completion_2003_tests tests/pstencil/light_completion_2003_tests.cpp)
    target_link_libraries(pstencil_light_completion_2003_tests PRIVATE halo2_cache)
    if(EXISTS "${PROJECT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
        add_test(NAME pstencil_image_free_armor_light_completion COMMAND pstencil_light_completion_2003_tests
            ${PROJECT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    endif()
    add_executable(pstencil_projected_constants_2003_tests tests/pstencil/projected_constants_2003_tests.cpp)
    target_link_libraries(pstencil_projected_constants_2003_tests PRIVATE halo2_pstencil halo2_cache)
    target_compile_options(pstencil_projected_constants_2003_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off;-fno-fast-math>)
    if(EXISTS "${PROJECT_SOURCE_DIR}/analysis/pstencil/projected-response-v1/reference.json" AND
       EXISTS "${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-light-inputs-v1/reference.json" AND
       EXISTS "${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-light-inputs-v1/opening/inputs.json")
        add_test(NAME pstencil_original_projected_constants COMMAND pstencil_projected_constants_2003_tests
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/projected-response-v1/reference.json
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-light-inputs-v1/reference.json
            ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-light-inputs-v1/opening)
    endif()
    add_executable(pstencil_audit_light_response_inputs tools/pstencil/audit_light_response_inputs.cpp)
    target_link_libraries(pstencil_audit_light_response_inputs PRIVATE halo2_systems)
    add_executable(pstencil_reference_light_inputs_tests tests/pstencil/reference_light_inputs_tests.cpp)
    target_link_libraries(pstencil_reference_light_inputs_tests PRIVATE halo2_systems)
    add_executable(pstencil_light_response_2003_tests tests/pstencil/light_response_2003_tests.cpp)
    target_link_libraries(pstencil_light_response_2003_tests PRIVATE halo2_pstencil Vulkan::Vulkan)
    target_compile_options(pstencil_light_response_2003_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/light_response_2003.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/tests/pstencil/light_response_2003.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/light_response_2003.comp.spv
        DEPENDS tests/pstencil/light_response_2003.comp shaders/combiner.glsl VERBATIM)
    add_custom_target(pstencil_light_response_2003_shader DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/light_response_2003.comp.spv)
    add_dependencies(pstencil_light_response_2003_tests pstencil_light_response_2003_shader)
    set(PSTENCIL_LIGHT_CONSTRUCTION_REFERENCE
        "${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/light-construction-v3/reference.json"
        CACHE FILEPATH "Retained original 2003 light-constructor oracle for optional GPU checks")
    set(PSTENCIL_LIGHT_RGB_REFERENCE
        "${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-light-inputs-v1/sphere-colors-v2.json"
        CACHE FILEPATH "Retained original 2003 RGB packing oracle for optional boundary checks")
    if(EXISTS "${PSTENCIL_LIGHT_CONSTRUCTION_REFERENCE}")
        set(pstencil_light_rgb_arguments)
        if(EXISTS "${PSTENCIL_LIGHT_RGB_REFERENCE}")
            list(APPEND pstencil_light_rgb_arguments "${PSTENCIL_LIGHT_RGB_REFERENCE}")
        endif()
        add_test(NAME pstencil_original_light_response_gpu COMMAND pstencil_light_response_2003_tests
            "${PSTENCIL_LIGHT_CONSTRUCTION_REFERENCE}"
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/light_response_2003.comp.spv
            ${pstencil_light_rgb_arguments})
        unset(pstencil_light_rgb_arguments)
        set_tests_properties(pstencil_original_light_response_gpu PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
    endif()
    add_executable(pstencil_combined_environment_tests tests/pstencil/combined_environment_tests.cpp)
    target_link_libraries(pstencil_combined_environment_tests PRIVATE halo2_cache)
    add_test(NAME pstencil_combined_visor_inputs COMMAND pstencil_combined_environment_tests
        ${PROJECT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/projected_fog_alpha.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/tests/pstencil/projected_fog_alpha.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/projected_fog_alpha.comp.spv
        COMMAND ${SPIRV_VALIDATOR} ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/projected_fog_alpha.comp.spv
        DEPENDS tests/pstencil/projected_fog_alpha.comp shaders/combiner.glsl VERBATIM)
    add_custom_target(pstencil_projected_fog_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/projected_fog_alpha.comp.spv)
    add_executable(pstencil_projected_fog_tests tests/pstencil/projected_fog_alpha_tests.cpp)
    target_link_libraries(pstencil_projected_fog_tests PRIVATE halo2_cache Vulkan::Vulkan)
    add_dependencies(pstencil_projected_fog_tests pstencil_projected_fog_shader)
    add_test(NAME pstencil_projected_fog_alpha COMMAND pstencil_projected_fog_tests
        ${PROJECT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/projected_fog_alpha.comp.spv)
    set_tests_properties(pstencil_projected_fog_alpha PROPERTIES RUN_SERIAL TRUE)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/tests/pstencil/environment.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv
        DEPENDS tests/pstencil/environment.comp shaders/combiner.glsl shaders/pstencil/specular_mask.glsl VERBATIM)
    add_custom_target(pstencil_environment_test_shader DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv)
    add_executable(pstencil_environment_tests tests/pstencil/environment_tests.cpp)
    target_link_libraries(pstencil_environment_tests PRIVATE halo2_cache Vulkan::Vulkan)
    add_dependencies(pstencil_environment_tests pstencil_environment_test_shader)
    add_executable(pstencil_adaptation_tests tests/pstencil/adaptation_tests.cpp)
    target_link_libraries(pstencil_adaptation_tests PRIVATE halo2_systems Vulkan::Vulkan)
    add_dependencies(pstencil_adaptation_tests pstencil_environment_test_shader)
    add_test(NAME pstencil_armor_adaptation COMMAND pstencil_adaptation_tests
        ${PROJECT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv
        ${PROJECT_SOURCE_DIR}/assets/disc/maps/coagulation.map)
    add_executable(pstencil_detail_tests tests/pstencil/detail_tests.cpp)
    target_link_libraries(pstencil_detail_tests PRIVATE halo2_cache Vulkan::Vulkan)
    add_dependencies(pstencil_detail_tests pstencil_environment_test_shader)
    add_test(NAME pstencil_elite_detail COMMAND pstencil_detail_tests
        ${PROJECT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv)
    add_test(NAME pstencil_environment_response COMMAND pstencil_environment_tests
        ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/fallback-environment.json
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/pstencil/environment.comp.spv
        ${PROJECT_SOURCE_DIR}/analysis/pstencil/2003-presentation/default_environment_map-resident-0.bin
        ${PROJECT_SOURCE_DIR}/analysis/pstencil/reference-environment-parameters/evidence.json)
    add_executable(pstencil_audit_caster_resources tools/pstencil/audit_caster_resources.cpp)
    target_link_libraries(pstencil_audit_caster_resources PRIVATE halo2_pstencil_render)
    add_executable(pstencil_audit_campaign_materials tools/pstencil/audit_campaign_materials.cpp)
    target_link_libraries(pstencil_audit_campaign_materials PRIVATE halo2_cache)
    add_executable(pstencil_caster_tests tests/pstencil/caster_tests.cpp)
    target_link_libraries(pstencil_caster_tests PRIVATE halo2_pstencil_render)
    add_test(NAME pstencil_caster_faces COMMAND pstencil_caster_tests ${PROJECT_SOURCE_DIR}/assets/disc/maps/zanzibar.map)
    add_executable(pstencil_gameplay_capture tools/pstencil/capture_gameplay.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(pstencil_gameplay_capture PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
    target_compile_definitions(pstencil_gameplay_capture PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_dependencies(pstencil_gameplay_capture halo2_shaders)
    add_executable(pstencil_point_lights_tests tests/pstencil/point_lights_tests.cpp)
    target_link_libraries(pstencil_point_lights_tests PRIVATE halo2_systems)
    add_test(NAME pstencil_plasma_point_light COMMAND pstencil_point_lights_tests ${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set(PSTENCIL_GAMEPLAY_XBE "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        CACHE FILEPATH "TU1.5 gameplay image for pStencil native attachment checks")
    add_executable(pstencil_projected_constants_tests tests/pstencil/projected_constants_tests.cpp)
    target_link_libraries(pstencil_projected_constants_tests PRIVATE halo2_systems)
    if(TARGET pstencil_projected_vertex_attributes_gpu_tests)
        add_test(NAME pstencil_projected_vertex_attributes_gpu COMMAND pstencil_projected_vertex_attributes_gpu_tests
            ${PROJECT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map "${PSTENCIL_GAMEPLAY_XBE}"
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv
            ${CMAKE_CURRENT_BINARY_DIR}/pstencil-projected-attribute-results)
        set_tests_properties(pstencil_projected_vertex_attributes_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RUN_SERIAL TRUE)
    endif()
    add_executable(pstencil_attached_lights_tests tests/pstencil/attached_lights_tests.cpp)
    target_link_libraries(pstencil_attached_lights_tests PRIVATE halo2_systems)
    add_executable(pstencil_native_incident_tests tests/pstencil/native_incident_tests.cpp)
    target_link_libraries(pstencil_native_incident_tests PRIVATE halo2_systems halo2_pstencil_render)
    add_executable(pstencil_scenario_lights_tests tests/pstencil/scenario_lights_tu15_tests.cpp)
    target_link_libraries(pstencil_scenario_lights_tests PRIVATE halo2_systems)
    if(EXISTS "${PSTENCIL_GAMEPLAY_XBE}")
        add_test(NAME pstencil_native_scenario_lights COMMAND pstencil_scenario_lights_tests
            "${PSTENCIL_GAMEPLAY_XBE}" ${PROJECT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
            ${CMAKE_CURRENT_BINARY_DIR}/pstencil-scenario-light-results)
        add_test(NAME pstencil_projected_resident_constants COMMAND pstencil_projected_constants_tests
            "${PSTENCIL_GAMEPLAY_XBE}" ${PROJECT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
        add_test(NAME pstencil_native_attached_light COMMAND pstencil_attached_lights_tests
            "${PSTENCIL_GAMEPLAY_XBE}" ${PROJECT_SOURCE_DIR}/assets/disc/maps/coagulation.map)
        add_test(NAME pstencil_native_cinematic_incident COMMAND pstencil_native_incident_tests
            "${PSTENCIL_GAMEPLAY_XBE}" ${PROJECT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map)
    endif()
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
    target_link_libraries(pstencil_ui_tests PRIVATE halo2_renderer)
    target_compile_definitions(pstencil_ui_tests PRIVATE HALO2_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME pstencil_live_graphics_selector COMMAND pstencil_ui_tests
        ${CMAKE_CURRENT_BINARY_DIR}/pstencil/ui-results)
    if(EXISTS "${PROJECT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
        set_tests_properties(pstencil_live_graphics_selector pstencil_real_map_roundtrip PROPERTIES
            ENVIRONMENT "VK_LAYER_PATH=${PROJECT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
    endif()
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
