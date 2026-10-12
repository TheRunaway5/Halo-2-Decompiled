# Same retained bank and native resource owners for the TU1.5 live world.
target_sources(halo2_systems PRIVATE src/physics_live_world_tu15.cpp src/source_runtime_cache_tags_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics_live_support_tu15_generated.cpp)
if(BUILD_TESTING)
    add_executable(physics_live_world_tu15_tests tests/physics_live_world_tu15_tests.cpp)
    target_link_libraries(physics_live_world_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_physics_live_world_tu15 COMMAND physics_live_world_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_physics_live_world_tu15 PROPERTIES TIMEOUT 120)
endif()
