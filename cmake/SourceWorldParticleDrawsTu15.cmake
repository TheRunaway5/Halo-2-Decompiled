target_sources(halo2_systems PRIVATE src/source_world_particle_draws_tu15.cpp)
if(BUILD_TESTING)
    add_executable(source_world_particle_draws_tu15_tests
        tests/source_world_particle_draws_tu15_tests.cpp)
    target_link_libraries(source_world_particle_draws_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_tu15_source_swarm_frame_draws
        COMMAND source_world_particle_draws_tu15_tests "${HALO2_TU15_GAMEPLAY_XBE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    set_tests_properties(native_tu15_source_swarm_frame_draws PROPERTIES SKIP_RETURN_CODE 77)
endif()
