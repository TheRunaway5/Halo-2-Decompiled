if(BUILD_TESTING)
    add_executable(source_map_damage_effect_tests tests/source_map_damage_effect_tests.cpp)
    target_link_libraries(source_map_damage_effect_tests PRIVATE halo2_systems)
    foreach(presentation IN ITEMS 0 1)
        add_test(NAME native_tu15_map_damage_effect_${presentation} COMMAND source_map_damage_effect_tests
            "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/triplicate.map" ${presentation})
        set_tests_properties(native_tu15_map_damage_effect_${presentation} PROPERTIES SKIP_RETURN_CODE 77
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endforeach()
endif()
