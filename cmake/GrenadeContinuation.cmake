if(BUILD_TESTING)
    add_executable(grenade_continuation_tests tests/grenade_continuation_tests.cpp)
    target_link_libraries(grenade_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_grenade_continuation COMMAND grenade_continuation_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_grenade_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
