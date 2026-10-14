target_sources(halo2_cache PRIVATE src/source_particle_sprite_context.cpp)
if(BUILD_TESTING)
    add_executable(source_particle_sprite_context_tests
        tests/source_particle_sprite_context_tests.cpp)
    target_link_libraries(source_particle_sprite_context_tests PRIVATE
        halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_particle_sprite_context
        COMMAND source_particle_sprite_context_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_particle_sprite_context_tu15.jsonl)
    set_tests_properties(original_tu15_particle_sprite_context PROPERTIES SKIP_RETURN_CODE 77)
endif()
