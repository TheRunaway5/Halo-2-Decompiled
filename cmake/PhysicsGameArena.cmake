# Complete original TU1.5 arena, async queue/worker and CRC startup.
target_sources(halo2_physics_world_runtime PRIVATE src/physics_game_arena_runtime_generated.cpp)
target_sources(halo2_systems PRIVATE src/physics_game_arena_native.cpp)
if(BUILD_TESTING)
    add_executable(physics_game_arena_tests tests/physics_game_arena_tests.cpp)
    target_link_libraries(physics_game_arena_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME native_physics_game_arena_tu15 COMMAND physics_game_arena_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/original-v2/report.json
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_physics_game_arena_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(physics_game_arena_continuation_tests tests/physics_game_arena_continuation_tests.cpp)
    target_link_libraries(physics_game_arena_continuation_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_physics_game_arena_continuation_tu15 COMMAND physics_game_arena_continuation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/original-v2/report.json
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_physics_game_arena_continuation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
