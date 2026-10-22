target_sources(halo2_cache PRIVATE src/rendering/effects/source_particle_sprite_context.cpp)
if(BUILD_TESTING)
    add_executable(mcc_particle_sprite_context_tests tests/mcc_particle_sprite_context_tests.cpp)
    target_link_libraries(mcc_particle_sprite_context_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    set(HALO2_MCC_PARTICLE_CONTEXT_MAP "" CACHE FILEPATH "Optional prepared MCC High Charity for retained sprite selector validation")
    add_test(NAME mcc_particle_sprite_context_forwarding COMMAND mcc_particle_sprite_context_tests
        "${HALO2_MCC_PARTICLE_CONTEXT_MAP}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_particle_sprite_context_forwarded_tu15.json")
    set_tests_properties(mcc_particle_sprite_context_forwarding PROPERTIES SKIP_RETURN_CODE 77)
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
