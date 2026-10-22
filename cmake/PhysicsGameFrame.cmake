# Original outer frame, including source FP controls and object synchronization.
target_sources(halo2_physics_world_runtime PRIVATE src/physics/physics_game_frame_runtime.cpp)
if(BUILD_TESTING)
    add_executable(physics_game_frame_runtime_tests tests/physics_game_frame_runtime_tests.cpp)
    target_link_libraries(physics_game_frame_runtime_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
    set(game_frame_evidence ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-game-frame-runtime-20261011)
    add_test(NAME native_physics_game_frame_prefix COMMAND physics_game_frame_runtime_tests
        ${game_frame_evidence}/hardware-prefix-verified/fixture.json 0030bb22)
    add_test(NAME native_physics_game_frame COMMAND physics_game_frame_runtime_tests
        ${game_frame_evidence}/hardware-frame-verified/fixture.json)
    add_test(NAME native_physics_game_frame_fp_controls COMMAND physics_game_frame_runtime_tests
        ${game_frame_evidence}/hardware-fp-verified/fixture.json)
    set_tests_properties(native_physics_game_frame_prefix native_physics_game_frame
        native_physics_game_frame_fp_controls PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
