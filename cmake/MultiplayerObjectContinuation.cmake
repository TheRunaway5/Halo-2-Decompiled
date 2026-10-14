if(BUILD_TESTING)
    add_executable(multiplayer_object_continuation_tests tests/multiplayer_object_continuation_tests.cpp)
    target_link_libraries(multiplayer_object_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_multiplayer_object_continuation COMMAND multiplayer_object_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_multiplayer_object_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
