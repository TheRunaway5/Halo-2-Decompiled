target_sources(halo2_cache PRIVATE src/rendering/effects/source_particle_sampler.cpp)
if(BUILD_TESTING)
    add_executable(source_particle_sampler_tests tests/source_particle_sampler_tests.cpp)
    target_link_libraries(source_particle_sampler_tests PRIVATE halo2_cache)
    add_test(NAME original_tu15_particle_sampler COMMAND source_particle_sampler_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/distortion-live-20261014/sampler-original.tsv")
    set_tests_properties(original_tu15_particle_sampler PROPERTIES SKIP_RETURN_CODE 77)
endif()
