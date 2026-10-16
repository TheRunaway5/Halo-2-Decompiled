target_sources(halo2_systems PRIVATE src/multiplayer_continuation.cpp src/source_match_authority.cpp src/source_execution_phase.cpp)
target_sources(halo2_systems PRIVATE src/source_execution_calls.cpp src/source_execution_lens.cpp)
set_source_files_properties(src/source_execution_calls.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost-pfr/include")
if(BUILD_TESTING)
    add_custom_target(online_source_replay_qualification DEPENDS
        halo2_native source_execution_inputs_tests source_execution_codec_tests source_runtime_shared_backing_tests source_execution_inputs_native_tests
        source_execution_calls_native_tests source_execution_abi_native_tests
        source_execution_live_model_native_tests source_execution_render_commands_native_tests
        source_execution_particle_native_tests source_execution_shadow_storage_native_tests source_world_particle_draws_tu15_tests
        source_player_session_departure_tu15_tests source_match_local_rebind_tests
        source_match_network_departure_tests source_authority_receiver_tests source_authority_codec_tests
        source_replay_batch_tests source_authority_replay_tests source_authority_publisher_tests source_runtime_memory_tests)
    add_executable(source_execution_shadow_storage_native_tests tests/source_execution_shadow_storage_native_tests.cpp)
    target_link_libraries(source_execution_shadow_storage_native_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_execution_shadow_storage_native_tu15 COMMAND source_execution_shadow_storage_native_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_execution_shadow_storage_native_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_execution_render_commands_native_tests tests/source_execution_render_commands_native_tests.cpp)
    target_link_libraries(source_execution_render_commands_native_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_execution_render_commands_native_tu15 COMMAND source_execution_render_commands_native_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_execution_render_commands_native_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_execution_particle_native_tests tests/source_execution_particle_native_tests.cpp)
    target_link_libraries(source_execution_particle_native_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_execution_particle_native_tu15 COMMAND source_execution_particle_native_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    set_tests_properties(source_execution_particle_native_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 330)
    add_executable(source_execution_live_model_native_tests tests/source_execution_live_model_native_tests.cpp)
    target_link_libraries(source_execution_live_model_native_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_execution_live_model_native_tu15 COMMAND source_execution_live_model_native_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_execution_live_model_native_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_execution_abi_native_tests tests/source_execution_abi_native_tests.cpp)
    target_link_libraries(source_execution_abi_native_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_execution_abi_native_tu15 COMMAND source_execution_abi_native_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_execution_abi_native_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_execution_calls_native_tests tests/source_execution_calls_native_tests.cpp)
    target_link_libraries(source_execution_calls_native_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_execution_calls_native_tu15 COMMAND source_execution_calls_native_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_execution_calls_native_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_player_session_departure_tu15_tests tests/source_player_session_departure_tu15_tests.cpp)
    target_link_libraries(source_player_session_departure_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_player_session_departure COMMAND source_player_session_departure_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_session_departure_tu15_original.json")
    set_tests_properties(original_tu15_player_session_departure PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_executable(native_melee_continuation_resources_tests tests/native_melee_continuation_resources_tests.cpp)
    target_include_directories(native_melee_continuation_resources_tests PRIVATE src)
    target_link_libraries(native_melee_continuation_resources_tests PRIVATE halo2_systems)
    add_test(NAME native_melee_continuation_resources_tu15 COMMAND native_melee_continuation_resources_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_melee_continuation_resources_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(source_match_local_rebind_tests tests/source_match_local_rebind_tests.cpp)
    target_link_libraries(source_match_local_rebind_tests PRIVATE halo2_systems)
    add_test(NAME source_match_local_rebind_tu15 COMMAND source_match_local_rebind_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_match_local_rebind_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_match_network_departure_tests tests/source_match_network_departure_tests.cpp)
    target_link_libraries(source_match_network_departure_tests PRIVATE halo2_systems)
    add_test(NAME source_match_network_departure_tu15 COMMAND source_match_network_departure_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_match_network_departure_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_player_departure_tu15_tests tests/source_player_departure_tu15_tests.cpp)
    target_link_libraries(source_player_departure_tu15_tests PRIVATE halo2_systems)
    add_test(NAME source_player_departure_tu15 COMMAND source_player_departure_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_player_departure_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(multiplayer_continuation_tests tests/multiplayer_continuation_tests.cpp)
    target_link_libraries(multiplayer_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_match_continuation COMMAND multiplayer_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_match_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_match_continuation_tests tests/source_match_continuation_tests.cpp)
    target_link_libraries(source_match_continuation_tests PRIVATE halo2_systems)
    add_test(NAME source_match_continuation_tu15 COMMAND source_match_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_match_continuation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME source_match_active_continuation_tu15 COMMAND source_match_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map" --active)
    set_tests_properties(source_match_active_continuation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME source_match_departure_continuation_tu15 COMMAND source_match_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map" --departure)
    set_tests_properties(source_match_departure_continuation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
