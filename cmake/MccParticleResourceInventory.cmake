if(BUILD_TESTING)
    set(HALO2_MCC_PARTICLE_INVENTORY_MAP "" CACHE FILEPATH
        "Prepared MCC map for complete particle material/effect dependency inventory")
    add_executable(mcc_particle_resource_inventory_tests tests/mcc_particle_resource_inventory_tests.cpp)
    target_link_libraries(mcc_particle_resource_inventory_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_particle_resource_inventory COMMAND mcc_particle_resource_inventory_tests
        "${HALO2_MCC_PARTICLE_INVENTORY_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/mcc-particle-resource-inventory.json")
    set_tests_properties(mcc_particle_resource_inventory PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    add_executable(mcc_particle_default_bitmap_tests tests/mcc_particle_default_bitmap_tests.cpp)
    target_link_libraries(mcc_particle_default_bitmap_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME particle_default_bitmap_selection COMMAND mcc_particle_default_bitmap_tests --unit)
    add_test(NAME mcc_particle_default_bitmap COMMAND mcc_particle_default_bitmap_tests
        "${HALO2_MCC_PARTICLE_INVENTORY_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/mcc-particle-default-bitmap.json")
    set_tests_properties(particle_default_bitmap_selection mcc_particle_default_bitmap
        PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(mcc_particle_pc_projection_tests tests/mcc_particle_pc_projection_tests.cpp)
    target_link_libraries(mcc_particle_pc_projection_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME particle_pc_projection_controls COMMAND mcc_particle_pc_projection_tests --unit)
    set_tests_properties(particle_pc_projection_controls PROPERTIES TIMEOUT 120)
    if(HALO2_MCC_PARTICLE_INVENTORY_MAP)
        get_filename_component(particle_inventory_maps "${HALO2_MCC_PARTICLE_INVENTORY_MAP}" DIRECTORY)
        foreach(level IN ITEMS needle street_sweeper)
            if(EXISTS "${particle_inventory_maps}/${level}.map")
                add_test(NAME mcc_particle_pc_projection_${level} COMMAND mcc_particle_pc_projection_tests
                    "${particle_inventory_maps}/${level}.map"
                    "${CMAKE_CURRENT_BINARY_DIR}/mcc-particle-pc-projection-${level}.json")
                set_tests_properties(mcc_particle_pc_projection_${level} PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
            endif()
        endforeach()
    endif()
endif()
