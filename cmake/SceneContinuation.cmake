target_sources(halo2_systems PRIVATE src/runtime/scene_continuation.cpp)
if(BUILD_TESTING)
    add_executable(source_trigger_volume_tu15_tests tests/source_trigger_volume_tu15_tests.cpp)
    target_link_libraries(source_trigger_volume_tu15_tests PRIVATE halo2_systems)
    add_test(NAME source_trigger_volume_tu15 COMMAND source_trigger_volume_tu15_tests)
    add_executable(scene_continuation_tests tests/scene_continuation_tests.cpp)
    target_link_libraries(scene_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_scene_continuation COMMAND scene_continuation_tests)
    add_test(NAME native_scene_tu15_continuation COMMAND scene_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_scene_tu15_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
