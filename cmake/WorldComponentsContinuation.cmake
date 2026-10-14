target_sources(halo2_systems PRIVATE src/prop_continuation.cpp src/scenery_collision_continuation.cpp src/device_continuation.cpp src/world_continuation_bindings.cpp)
if(BUILD_TESTING)
    add_executable(device_continuation_tests tests/device_continuation_tests.cpp)
    target_link_libraries(device_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_device_continuation COMMAND device_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map")
    set_tests_properties(native_device_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(world_components_continuation_tests tests/world_components_continuation_tests.cpp)
    target_link_libraries(world_components_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_world_components_continuation COMMAND world_components_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map")
    set_tests_properties(native_world_components_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
