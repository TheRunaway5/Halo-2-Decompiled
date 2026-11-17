# Native TU1.5 PVS closure (1532A0/170C80/170FB0/44680): seeded comparison
# against the pinned translated bodies, sanitizer replay and mutation screen.
if(BUILD_TESTING)
    add_custom_target(native_pvs_flood_tests)
    add_test(NAME native_tu15_pvs_flood
        COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_pvs_flood_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-pvs-flood.json" --jobs 2)
    set_tests_properties(native_tu15_pvs_flood PROPERTIES TIMEOUT 1800)
endif()
