# Fresh original physics pools, separate from captured world/contact state.
target_sources(halo2_physics_world_runtime PRIVATE src/physics_pool_startup_runtime_generated.cpp)
target_sources(halo2_systems PRIVATE src/physics_pool_host_storage.cpp)
if(BUILD_TESTING)
    add_executable(physics_pool_startup_tests tests/physics_pool_startup_tests.cpp)
    target_link_libraries(physics_pool_startup_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_physics_pool_startup_tu15 COMMAND physics_pool_startup_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012/original-v4/fixture.json
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_physics_pool_startup_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
