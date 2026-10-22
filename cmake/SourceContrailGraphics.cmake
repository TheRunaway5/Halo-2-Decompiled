target_sources(halo2_cache PRIVATE src/rendering/effects/source_contrail_state.cpp)
if(BUILD_TESTING)
    add_executable(contrail_state_tu15_tests tests/contrail_state_tu15_tests.cpp)
    target_link_libraries(contrail_state_tu15_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_contrail_pixel_stores COMMAND contrail_state_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/contrail_state_tu15_original.json")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map")
    set(SOURCE_CONTRAIL_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_custom_command(OUTPUT ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail_program.glsl
        ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail_program.hpp ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail_program.json
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_source_contrail_program_tu15.py
            --cache ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map
            --output ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail_program.glsl
        DEPENDS tools/generate_source_contrail_program_tu15.py tools/generate_particle_graphics_program.py
            tools/generate_particle_vertex_probe.py tools/audit_rendering.py assets/disc/maps/zanzibar.map VERBATIM)
    add_custom_target(source_contrail_program DEPENDS ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail_program.hpp)
    foreach(stage vert geom frag)
        add_custom_command(OUTPUT ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail.${stage}.spv
            COMMAND ${GLSLANG_VALIDATOR} -V -I${SOURCE_CONTRAIL_SHADER_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source_contrail.${stage}
                -o ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail.${stage}.spv
            DEPENDS shaders/source_contrail.${stage} shaders/combiner.glsl shaders/world_depth_fragment.glsl
                ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail_program.glsl VERBATIM)
    endforeach()
    add_custom_target(source_contrail_shaders DEPENDS ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail.vert.spv
        ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail.geom.spv ${SOURCE_CONTRAIL_SHADER_DIR}/source_contrail.frag.spv)
    target_sources(halo2_systems PRIVATE src/rendering/effects/source_contrail_draw.cpp)
    set_source_files_properties(src/rendering/effects/source_contrail_draw.cpp PROPERTIES INCLUDE_DIRECTORIES "${SOURCE_CONTRAIL_SHADER_DIR}")
    add_dependencies(halo2_systems source_contrail_program)
    target_sources(halo2_renderer PRIVATE src/rendering/effects/source_contrail_renderer.cpp)
    target_compile_definitions(halo2_renderer PRIVATE HALO2_SOURCE_CONTRAILS_AVAILABLE=1)
    target_compile_definitions(halo2_native PRIVATE HALO2_SOURCE_CONTRAILS_AVAILABLE=1)
    if(TARGET physics_multiplayer_acceptance)
        target_compile_definitions(physics_multiplayer_acceptance PRIVATE HALO2_SOURCE_CONTRAILS_AVAILABLE=1)
        add_dependencies(physics_multiplayer_acceptance source_contrail_shaders)
    endif()
    add_dependencies(halo2_renderer source_contrail_shaders)
endif()

if(BUILD_TESTING AND TARGET source_contrail_shaders)
    add_executable(live_contrail_packet_gpu_tests tests/live_contrail_packet_gpu_tests.cpp)
    target_link_libraries(live_contrail_packet_gpu_tests PRIVATE halo2_renderer)
    target_compile_definitions(live_contrail_packet_gpu_tests PRIVATE HALO2_SHADER_DIR="${SOURCE_CONTRAIL_SHADER_DIR}")
    add_dependencies(live_contrail_packet_gpu_tests source_contrail_shaders)
endif()
