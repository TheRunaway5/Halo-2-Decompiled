target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/multiplayer_continuation.cpp src/runtime/source_match_authority.cpp src/runtime/source_execution_phase.cpp)
target_sources(halo2_systems PRIVATE src/runtime/source_execution_calls.cpp src/runtime/source_execution_lens.cpp)
set_source_files_properties(src/runtime/source_execution_calls.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost-pfr/include")
if(BUILD_TESTING)
    add_executable(effect_location_admission_tu15_tests tests/effect_location_admission_tu15_tests.cpp)
    target_include_directories(effect_location_admission_tu15_tests PRIVATE include)
    add_test(NAME original_tu15_effect_location_admission COMMAND effect_location_admission_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_location_world_tu15.original.bin")
    add_executable(multiplayer_replica_refresh_tests tests/multiplayer_replica_refresh_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(multiplayer_replica_refresh_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
    target_compile_definitions(multiplayer_replica_refresh_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_dependencies(multiplayer_replica_refresh_tests halo2_shaders)
    add_test(NAME native_replica_refresh_runtime COMMAND multiplayer_replica_refresh_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(native_replica_refresh_runtime PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240 RUN_SERIAL TRUE)
    add_executable(multiplayer_effect_history_tests tests/multiplayer_effect_history_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(multiplayer_effect_history_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
    target_compile_definitions(multiplayer_effect_history_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_dependencies(multiplayer_effect_history_tests halo2_shaders)
    add_test(NAME native_online_effect_history COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/multiplayer_effect_history_http_test.py" $<TARGET_FILE:multiplayer_effect_history_tests>)
    set_tests_properties(native_online_effect_history PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 270 RUN_SERIAL TRUE
        LABELS "offline-reference")
    set(HALO2_MCC_GAMEPLAY_TEST_CACHE "" CACHE STRING "Explicit selected prepared MCC cache for native ONLINE gameplay history tests; empty disables this route")
    if(NOT "${HALO2_MCC_GAMEPLAY_TEST_CACHE}" STREQUAL "" AND
       NOT "${HALO2_MCC_GAMEPLAY_TEST_CACHE}" STREQUAL "auto")
        get_filename_component(_halo2_mcc_gameplay_cache "${HALO2_MCC_GAMEPLAY_TEST_CACHE}"
            ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME native_online_effect_history_mcc COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/multiplayer_effect_history_http_test.py" $<TARGET_FILE:multiplayer_effect_history_tests>
            --mcc-root "${_halo2_mcc_gameplay_cache}")
        set_tests_properties(native_online_effect_history_mcc PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 270 RUN_SERIAL TRUE
            LABELS "selected-mcc-native")
        add_test(NAME native_mcc_prompt_font_owner COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_native_prompt_fonts.py"
            --mcc-root "${_halo2_mcc_gameplay_cache}")
        set_tests_properties(native_mcc_prompt_font_owner PROPERTIES TIMEOUT 60 LABELS "selected-mcc-font")
    endif()
    add_test(NAME native_audio_late_join COMMAND multiplayer_audio_late_join_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(native_audio_late_join PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240 RUN_SERIAL TRUE
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_custom_target(online_source_replay_qualification DEPENDS
        halo2_native source_execution_inputs_tests source_execution_codec_tests source_runtime_shared_backing_tests source_execution_inputs_native_tests
        source_execution_calls_native_tests source_execution_abi_native_tests source_effect_frame_inactive_native_tests
        source_execution_live_model_native_tests source_execution_render_commands_native_tests
        source_execution_particle_native_tests source_execution_shadow_storage_native_tests source_world_particle_draws_tu15_tests
        source_player_session_departure_tu15_tests source_match_local_rebind_tests
        source_match_network_departure_tests source_host_continuation_tests source_host_role_rebind_tests source_authority_receiver_tests source_authority_codec_tests
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
    add_executable(source_effect_frame_inactive_native_tests tests/source_effect_frame_inactive_native_tests.cpp)
    target_link_libraries(source_effect_frame_inactive_native_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_effect_frame_inactive_native_tu15 COMMAND source_effect_frame_inactive_native_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_effect_frame_inactive_native_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
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
    add_executable(source_host_continuation_tests tests/source_host_continuation_tests.cpp)
    add_executable(multiplayer_snapshot_origin_tests tests/multiplayer_snapshot_origin_tests.cpp)
    target_link_libraries(multiplayer_snapshot_origin_tests PRIVATE halo2_renderer)
    add_test(NAME native_snapshot_origin_tu15 COMMAND multiplayer_snapshot_origin_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_snapshot_origin_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    if(NOT "${HALO2_MCC_GAMEPLAY_TEST_CACHE}" STREQUAL "" AND
       NOT "${HALO2_MCC_GAMEPLAY_TEST_CACHE}" STREQUAL "auto")
        add_test(NAME native_snapshot_origin_mcc COMMAND multiplayer_snapshot_origin_tests
            --mcc "${_halo2_mcc_gameplay_cache}/halo2/h2_maps_win64_dx11/lockout.map")
        set_tests_properties(native_snapshot_origin_mcc PROPERTIES TIMEOUT 240 RUN_SERIAL TRUE
            LABELS "selected-mcc-native;component;locomotion-continuity"
            ENVIRONMENT "SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
    endif()
    target_link_libraries(source_host_continuation_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_host_continuation_tu15 COMMAND source_host_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_host_continuation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_host_role_rebind_tests tests/source_host_role_rebind_tests.cpp)
    target_link_libraries(source_host_role_rebind_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_host_role_rebind_tu15 COMMAND source_host_role_rebind_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_host_role_rebind_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
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
