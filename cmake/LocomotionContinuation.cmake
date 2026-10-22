if(BUILD_TESTING)
    add_executable(native_locomotion_continuity_tests tests/native_locomotion_continuity_tests.cpp)
    target_link_libraries(native_locomotion_continuity_tests PRIVATE halo2_systems Threads::Threads)
    add_test(NAME native_locomotion_exact_continuity COMMAND native_locomotion_continuity_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(native_locomotion_exact_continuity PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
