target_sources(halo2_cache PRIVATE src/source_effect_seed_location.cpp)
if(MSVC)
    set_source_files_properties(src/source_effect_seed_location.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/source_effect_seed_location.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
    add_executable(source_effect_seed_location_tests tests/source_effect_seed_location_tests.cpp)
    target_link_libraries(source_effect_seed_location_tests PRIVATE halo2_cache)
    add_test(NAME original_tu15_map_effect_seed_locations COMMAND source_effect_seed_location_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/attachment-continuation-20261014/seed-locations/inputs.bin"
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/attachment-continuation-20261014/seed-locations/original.bin")
    set_tests_properties(original_tu15_map_effect_seed_locations PROPERTIES SKIP_RETURN_CODE 77)
    foreach(map IN ITEMS warlock deltatap)
        add_test(NAME original_tu15_map_particle_shape9_${map}
            COMMAND particle_birth_tests --tu15 "${HALO2_TU15_GAMEPLAY_XBE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/${map}.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/attachment-continuation-20261014/shape9-${map}/inputs.bin"
            "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/attachment-continuation-20261014/shape9-${map}/original.bin")
        set_tests_properties(original_tu15_map_particle_shape9_${map}
            PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
endif()
