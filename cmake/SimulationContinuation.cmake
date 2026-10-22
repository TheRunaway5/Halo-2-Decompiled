target_sources(halo2_systems PRIVATE src/gameplay/simulation_continuation.cpp src/physics/collision/collision_continuation.cpp)
if(BUILD_TESTING)
    add_executable(simulation_continuation_tests tests/simulation_continuation_tests.cpp)
    target_link_libraries(simulation_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_simulation_continuation COMMAND simulation_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_simulation_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
