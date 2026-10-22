target_sources(halo2_systems PRIVATE src/rendering/effects/source_particle_render_state_tu15.cpp)
if(BUILD_TESTING)
    add_executable(source_particle_render_state_tu15_tests
        tests/source_particle_render_state_tu15_tests.cpp)
    target_link_libraries(source_particle_render_state_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_particle_pool_render_inputs
        COMMAND source_particle_render_state_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_particle_render_state_tu15.json)
    set_tests_properties(original_tu15_particle_pool_render_inputs PROPERTIES SKIP_RETURN_CODE 77)
endif()
