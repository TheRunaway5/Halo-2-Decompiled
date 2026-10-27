target_sources(halo2_physics_world_runtime PRIVATE
    src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp
    src/gameplay/multiplayer/source_result_recording_support_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/native_tu15_multiplayer_data.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/native_tu15_multiplayer_branch_data.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_multiplayer_engine_entity_tu15.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_objective_presentation_tu15.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_objective_goal_geometry_tu15.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/objective_waypoints.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/objective_waypoint_memory_tu15.cpp src/network/system_link_session.cpp)
if(NOT MSVC)
    set_source_files_properties(src/gameplay/multiplayer/source_objective_goal_geometry_tu15.cpp
        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/audio/source_announcer_events_tu15.cpp src/audio/source_announcer_assets_tu15.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp
        src/gameplay/multiplayer/source_result_recording_support_tu15_generated.cpp
        PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp
        src/gameplay/multiplayer/source_result_recording_support_tu15_generated.cpp
        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()

if(BUILD_TESTING)
    add_executable(cloth_shader_tu15_tests tests/cloth_shader_tu15_tests.cpp)
    target_include_directories(cloth_shader_tu15_tests PRIVATE include src ${HALO2_TEST_JSON_INCLUDE})
    add_test(NAME original_cloth_shader_tu15 COMMAND cloth_shader_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cloth_shader_original_tu15.json")
    add_executable(objective_goal_geometry_tu15_tests tests/objective_goal_geometry_tu15_tests.cpp
        src/gameplay/multiplayer/source_objective_goal_geometry_tu15.cpp src/gameplay/multiplayer/source_king_boundary.cpp)
    target_include_directories(objective_goal_geometry_tu15_tests PRIVATE include src ${HALO2_TEST_JSON_INCLUDE})
    add_test(NAME original_objective_goal_geometry_tu15 COMMAND objective_goal_geometry_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/objective_goal_geometry_original_tu15.json")
    add_executable(objective_pc_variant_tests tests/objective_pc_variant_tests.cpp
        src/gameplay/multiplayer/source_game_variant.cpp src/ui/vista/vista_pcr_publication.cpp)
    target_include_directories(objective_pc_variant_tests PRIVATE include src ${HALO2_TEST_JSON_INCLUDE})
    add_test(NAME original_objective_pc_variant COMMAND objective_pc_variant_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/objective_pc_variant_original.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json")
    if(HALO2_SANITIZERS AND NOT MSVC)
        foreach(objective_test IN ITEMS objective_goal_geometry_tu15_tests objective_pc_variant_tests cloth_shader_tu15_tests)
            target_compile_options(${objective_test} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${objective_test} PRIVATE -fsanitize=address,undefined)
        endforeach()
    endif()
    add_executable(objective_audio_tu15_tests tests/objective_audio_tu15_tests.cpp
        src/audio/source_announcer_events_tu15.cpp src/audio/vista_announcer_queue.cpp)
    target_include_directories(objective_audio_tu15_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(objective_audio_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(objective_audio_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_objective_audio_tu15 COMMAND objective_audio_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/objective_audio_original_tu15.json")
    add_executable(frontend_objective_start_tu15_tests tests/frontend_objective_start_tu15_tests.cpp)
    target_include_directories(frontend_objective_start_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
    target_link_libraries(frontend_objective_start_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_frontend_objective_start_tu15 COMMAND frontend_objective_start_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json")
    set_tests_properties(original_frontend_objective_start_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(objective_native_tu15_tests tests/objective_native_tu15_tests.cpp)
    target_include_directories(objective_native_tu15_tests PRIVATE include src ${HALO2_TEST_JSON_INCLUDE})
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(objective_native_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(objective_native_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_objective_native_kernels_tu15 COMMAND objective_native_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/objective_native_original_tu15.json")
    add_executable(objective_native_owner_tu15_tests tests/objective_native_owner_tu15_tests.cpp
        tests/ctf_activation_test_support_generated.cpp
        src/gameplay/multiplayer/source_ctf_activation_tu15_generated.cpp
        src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp)
    target_include_directories(objective_native_owner_tu15_tests PRIVATE include src ${HALO2_TEST_JSON_INCLUDE})
    if(NOT MSVC)
        target_compile_options(objective_native_owner_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        if(HALO2_SANITIZERS)
            target_compile_options(objective_native_owner_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(objective_native_owner_tu15_tests PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
    add_test(NAME original_objective_native_owners_tu15 COMMAND objective_native_owner_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/objective_owners_original_tu15.json")
    add_test(NAME original_objective_native_overtime_tu15 COMMAND objective_native_owner_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/objective_overtime_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/objective-overtime-native-control.json")
    add_test(NAME original_objective_native_regions_tu15 COMMAND objective_native_owner_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/objective_regions_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/objective-regions-native-control.json")
    add_executable(result_recording_support_tu15_tests tests/result_recording_support_tu15_tests.cpp)
    target_include_directories(result_recording_support_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
    target_link_libraries(result_recording_support_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_result_recording_support_tu15 COMMAND result_recording_support_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/result_recording_support_original_tu15.json")
    set(HALO2_MCC_MULTIPLAYER_ENGINE_MAP "" CACHE FILEPATH
        "Imported MCC map for actual TU1.5 multiplayer engine integration")
    get_filename_component(HALO2_OBJECTIVE_TEST_MAPS "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" DIRECTORY)
    add_test(NAME native_mcc_frontend_objective_start_tu15 COMMAND frontend_objective_start_tu15_tests
        "${HALO2_OBJECTIVE_TEST_MAPS}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json")
    set_tests_properties(native_mcc_frontend_objective_start_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(ctf_assault_runtime_start_tu15_tests tests/ctf_assault_runtime_start_tu15_tests.cpp
        src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_include_directories(ctf_assault_runtime_start_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
    target_link_libraries(ctf_assault_runtime_start_tu15_tests PRIVATE halo2_renderer)
    add_executable(multiplayer_end_presentation_tu15_tests tests/multiplayer_end_presentation_tu15_tests.cpp
        src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(multiplayer_end_presentation_tu15_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    target_compile_definitions(multiplayer_end_presentation_tu15_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
    add_dependencies(multiplayer_end_presentation_tu15_tests halo2_shaders)
    foreach(end_preset IN ITEMS Slayer Oddball King "2 on 1" "CTF Classic" "Single Bomb" "3 Plots")
        string(TOLOWER "${end_preset}" end_name)
        string(REPLACE " " "_" end_name "${end_name}")
        add_test(NAME native_mcc_end_presentation_${end_name}_tu15 COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_multiplayer_end_presentation_tu15.py"
            $<TARGET_FILE:multiplayer_end_presentation_tu15_tests> "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" "${end_preset}")
        set_tests_properties(native_mcc_end_presentation_${end_name}_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 660 RUN_SERIAL TRUE)
    endforeach()
    add_executable(objective_remote_consumers_tu15_tests tests/objective_remote_consumers_tu15_tests.cpp
        src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(objective_remote_consumers_tu15_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    add_executable(ctf_assault_system_link_runtime_tests tests/ctf_assault_system_link_runtime_tests.cpp
        src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(ctf_assault_system_link_runtime_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME native_mcc_ctf_assault_system_link_runtime_tu15 COMMAND ctf_assault_system_link_runtime_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-system-link-runtime.json")
    set_tests_properties(native_mcc_ctf_assault_system_link_runtime_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(ctf_assault_system_link_return_tests tests/ctf_assault_system_link_return_tests.cpp)
    target_link_libraries(ctf_assault_system_link_return_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_mcc_ctf_assault_system_link_return_tu15 COMMAND ctf_assault_system_link_return_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-system-link-return.json")
    set_tests_properties(native_mcc_ctf_assault_system_link_return_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800 RUN_SERIAL TRUE ENVIRONMENT "H2_NET_PROFILE=complete")
    add_executable(ctf_assault_online_return_tests tests/ctf_assault_online_return_tests.cpp)
    target_link_libraries(ctf_assault_online_return_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_mcc_ctf_assault_online_return_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ctf_assault_online_return_http_test.py"
        $<TARGET_FILE:ctf_assault_online_return_tests> "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-online-return.json")
    set_tests_properties(native_mcc_ctf_assault_online_return_tu15 PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 2400 RUN_SERIAL TRUE ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
    add_test(NAME native_mcc_ctf_assault_remote_consumers_tu15 COMMAND objective_remote_consumers_tu15_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-remote-consumers.json")
    set_tests_properties(native_mcc_ctf_assault_remote_consumers_tu15 PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 2400 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(objective_goal_renderer_tests tests/objective_goal_renderer_tests.cpp)
    target_link_libraries(objective_goal_renderer_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME native_mcc_objective_goal_renderer_tu15 COMMAND objective_goal_renderer_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}")
    set_tests_properties(native_mcc_objective_goal_renderer_tu15 PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 300 RUN_SERIAL TRUE)
    add_test(NAME native_mcc_ctf_assault_runtime_start_tu15 COMMAND ctf_assault_runtime_start_tu15_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-runtime-start.json")
    set_tests_properties(native_mcc_ctf_assault_runtime_start_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    add_test(NAME native_mcc_ctf_assault_rendered_consumers_tu15 COMMAND ctf_assault_runtime_start_tu15_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-rendered-consumers.json")
    set_tests_properties(native_mcc_ctf_assault_rendered_consumers_tu15 PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 1800 RUN_SERIAL TRUE
        ENVIRONMENT "HALO2_OBJECTIVE_RENDER=1;SDL_AUDIODRIVER=dummy")
    add_test(NAME native_mcc_ctf_assault_lobby_return_tu15 COMMAND ctf_assault_runtime_start_tu15_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-lobby-return.json")
    set_tests_properties(native_mcc_ctf_assault_lobby_return_tu15 PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 600 ENVIRONMENT "HALO2_OBJECTIVE_REQUIRE_RETURN=1")
    add_executable(multiplayer_engines_world_tu15_tests tests/multiplayer_engines_world_tu15_tests.cpp)
    target_include_directories(multiplayer_engines_world_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
    target_link_libraries(multiplayer_engines_world_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_mcc_multiplayer_engines_tu15 COMMAND multiplayer_engines_world_tu15_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engines-world.json")
    set_tests_properties(native_mcc_multiplayer_engines_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(multiplayer_engine_network_tu15_tests tests/multiplayer_engine_network_tu15_tests.cpp)
        target_include_directories(multiplayer_engine_network_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_engine_network_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_mcc_multiplayer_engine_network_tu15 COMMAND multiplayer_engine_network_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engine-network.json")
        set_tests_properties(native_mcc_multiplayer_engine_network_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)
        add_test(NAME native_mcc_multiplayer_engine_network_slayer_tu15 COMMAND multiplayer_engine_network_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engine-network-slayer.json" Slayer)
        set_tests_properties(native_mcc_multiplayer_engine_network_slayer_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 360)
        add_executable(multiplayer_engine_entity_tu15_tests tests/multiplayer_engine_entity_tu15_tests.cpp)
        target_include_directories(multiplayer_engine_entity_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_engine_entity_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_mcc_multiplayer_engine_entity_tu15 COMMAND multiplayer_engine_entity_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engine-entity.json")
        set_tests_properties(native_mcc_multiplayer_engine_entity_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1200)
        add_test(NAME native_mcc_multiplayer_engine_entity_slayer_tu15 COMMAND multiplayer_engine_entity_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engine-entity-slayer.json" Slayer)
        set_tests_properties(native_mcc_multiplayer_engine_entity_slayer_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
        add_executable(multiplayer_round_wins_tu15_tests tests/multiplayer_round_wins_tu15_tests.cpp)
        target_include_directories(multiplayer_round_wins_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_round_wins_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_mcc_multiplayer_round_wins_tu15 COMMAND multiplayer_round_wins_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-round-wins.json")
        set_tests_properties(native_mcc_multiplayer_round_wins_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)
        add_test(NAME native_mcc_multiplayer_round_win_single_tu15 COMMAND multiplayer_round_wins_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-round-win-single.json" 0)
        set_tests_properties(native_mcc_multiplayer_round_win_single_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 360)
        add_executable(multiplayer_round_flow_tu15_tests tests/multiplayer_round_flow_tu15_tests.cpp)
        target_include_directories(multiplayer_round_flow_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_round_flow_tu15_tests PRIVATE halo2_systems)
        add_executable(multiplayer_round_network_tu15_tests tests/multiplayer_round_network_tu15_tests.cpp)
        target_include_directories(multiplayer_round_network_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_round_network_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_mcc_multiplayer_round_network_tu15 COMMAND multiplayer_round_network_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-round-network.json")
        set_tests_properties(native_mcc_multiplayer_round_network_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
        # This fixture compares complete source journals and byte-exact UDP state.
        set_property(TEST native_mcc_multiplayer_round_network_tu15 APPEND PROPERTY ENVIRONMENT_MODIFICATION "H2_NET_PROFILE=set:complete")
        add_test(NAME native_mcc_multiplayer_round_flow_tu15 COMMAND multiplayer_round_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-round-flow.json")
        set_tests_properties(native_mcc_multiplayer_round_flow_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)
        foreach(round_preset IN ITEMS Slayer Elimination)
            string(TOLOWER "${round_preset}" round_name)
            add_test(NAME native_mcc_multiplayer_round_${round_name}_tu15 COMMAND multiplayer_round_flow_tu15_tests
                "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
                "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-round-${round_name}.json" "${round_preset}")
            set_tests_properties(native_mcc_multiplayer_round_${round_name}_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
        endforeach()
        add_executable(multiplayer_engine_combat_tu15_tests tests/multiplayer_engine_combat_tu15_tests.cpp)
        target_include_directories(multiplayer_engine_combat_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_engine_combat_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_mcc_multiplayer_engine_combat_tu15 COMMAND multiplayer_engine_combat_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engine-combat.json")
        set_tests_properties(native_mcc_multiplayer_engine_combat_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1200)
        add_test(NAME native_mcc_multiplayer_engine_combat_slayer_tu15 COMMAND multiplayer_engine_combat_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engine-combat-slayer.json" "Slayer")
        set_tests_properties(native_mcc_multiplayer_engine_combat_slayer_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
        add_test(NAME native_mcc_multiplayer_role_win_tu15 COMMAND multiplayer_engine_combat_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-role-win.json" "3 on 1")
        set_tests_properties(native_mcc_multiplayer_role_win_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 360 ENVIRONMENT "HALO2_COMBAT_WIN_FLOW=1")
        add_test(NAME native_mcc_multiplayer_engine_event_wins_tu15 COMMAND multiplayer_engine_combat_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engine-event-wins.json")
        set_tests_properties(native_mcc_multiplayer_engine_event_wins_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_COMBAT_ENGINE_EVENT_WIN_FLOW=1")
        add_executable(multiplayer_role_flow_tu15_tests tests/multiplayer_role_flow_tu15_tests.cpp)
        target_include_directories(multiplayer_role_flow_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_role_flow_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_mcc_multiplayer_role_flow_tu15 COMMAND multiplayer_role_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-role-flow.json")
        set_tests_properties(native_mcc_multiplayer_role_flow_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)
        add_executable(multiplayer_zone_flow_tu15_tests tests/multiplayer_zone_flow_tu15_tests.cpp)
        target_include_directories(multiplayer_zone_flow_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_zone_flow_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_mcc_multiplayer_zone_flow_tu15 COMMAND multiplayer_zone_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-zone-flow.json")
        set_tests_properties(native_mcc_multiplayer_zone_flow_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800)
        add_test(NAME native_mcc_multiplayer_zone_wins_tu15 COMMAND multiplayer_zone_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-zone-wins.json")
        set_tests_properties(native_mcc_multiplayer_zone_wins_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_ZONE_WIN_FLOW=1")
        add_test(NAME native_mcc_multiplayer_hill_movement_tu15 COMMAND multiplayer_zone_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-hill-movement.json")
        set_tests_properties(native_mcc_multiplayer_hill_movement_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_ZONE_MOVEMENT=1")
        add_test(NAME native_mcc_multiplayer_territory_regions_tu15 COMMAND multiplayer_zone_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-territory-regions.json")
        set_tests_properties(native_mcc_multiplayer_territory_regions_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_ZONE_REGIONS=1")
        add_executable(multiplayer_objective_flow_tu15_tests tests/multiplayer_objective_flow_tu15_tests.cpp)
        target_include_directories(multiplayer_objective_flow_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_objective_flow_tu15_tests PRIVATE halo2_systems)
        foreach(kind IN ITEMS short touch)
            add_test(NAME native_mcc_ctf_assault_${kind}_returns_tu15 COMMAND multiplayer_objective_flow_tu15_tests
                "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
                "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-${kind}-returns.json")
            string(TOUPPER "${kind}" uppercase_kind)
            set_tests_properties(native_mcc_ctf_assault_${kind}_returns_tu15 PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 2400
                ENVIRONMENT "HALO2_OBJECTIVE_RETURN_FLOW=1;HALO2_OBJECTIVE_${uppercase_kind}_RETURN=1")
            add_test(NAME native_mcc_ctf_assault_${kind}_returns_reference_tu15 COMMAND multiplayer_objective_flow_tu15_tests
                "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
                "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-${kind}-returns-reference.json")
            set_tests_properties(native_mcc_ctf_assault_${kind}_returns_reference_tu15 PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 2400
                ENVIRONMENT "HALO2_OBJECTIVE_RETURN_FLOW=1;HALO2_OBJECTIVE_${uppercase_kind}_RETURN=1;HALO2_REFERENCE_OBJECTIVE_NATIVE_KERNELS=1")
            add_test(NAME native_mcc_ctf_assault_${kind}_returns_comparison_tu15 COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/compare_objective_native_matches_tu15.py"
                "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-${kind}-returns.json"
                "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-${kind}-returns-reference.json"
                "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-${kind}-returns-comparison.json")
            set_tests_properties(native_mcc_ctf_assault_${kind}_returns_comparison_tu15 PROPERTIES
                DEPENDS "native_mcc_ctf_assault_${kind}_returns_tu15;native_mcc_ctf_assault_${kind}_returns_reference_tu15")
        endforeach()
        add_executable(ctf_assault_match_tu15_tests tests/ctf_assault_match_tu15_tests.cpp)
        target_include_directories(ctf_assault_match_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(ctf_assault_match_tu15_tests PRIVATE halo2_systems OpenSSL::Crypto)
        add_test(NAME native_mcc_ctf_assault_full_lobby_audio_tu15 COMMAND ctf_assault_match_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-full-lobby-audio.json")
        set_tests_properties(native_mcc_ctf_assault_full_lobby_audio_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_OBJECTIVE_LOBBY=1;SDL_AUDIODRIVER=dummy")
        add_test(NAME native_mcc_ctf_assault_complete_matches_tu15 COMMAND ctf_assault_match_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-complete-matches.json")
        set_tests_properties(native_mcc_ctf_assault_complete_matches_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 3600)
        add_test(NAME native_mcc_ctf_assault_network_matches_tu15 COMMAND ctf_assault_match_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-network-matches.json")
        set_tests_properties(native_mcc_ctf_assault_network_matches_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 3600 ENVIRONMENT "HALO2_OBJECTIVE_NETWORK=1")
        add_test(NAME native_mcc_ctf_single_flag_network_tu15 COMMAND ctf_assault_match_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-single-flag-network.json")
        set_tests_properties(native_mcc_ctf_single_flag_network_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 1200 ENVIRONMENT "HALO2_OBJECTIVE_NETWORK=1;HALO2_OBJECTIVE_VARIANT=1 Flag CTF")
        foreach(path IN ITEMS native reference)
            add_test(NAME native_mcc_ctf_assault_edited_${path}_tu15 COMMAND ctf_assault_match_tu15_tests
                "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
                "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-edited-${path}.json")
            set_tests_properties(native_mcc_ctf_assault_edited_${path}_tu15 PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 3600 ENVIRONMENT "HALO2_OBJECTIVE_EDITED_RULES=1")
        endforeach()
        set_property(TEST native_mcc_ctf_assault_edited_reference_tu15 APPEND PROPERTY
            ENVIRONMENT "HALO2_REFERENCE_OBJECTIVE_NATIVE_KERNELS=1")
        add_test(NAME native_mcc_ctf_assault_edited_comparison_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/compare_objective_native_matches_tu15.py"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-edited-native.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-edited-reference.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-edited-comparison.json")
        set_tests_properties(native_mcc_ctf_assault_edited_comparison_tu15 PROPERTIES
            DEPENDS "native_mcc_ctf_assault_edited_native_tu15;native_mcc_ctf_assault_edited_reference_tu15")
        add_test(NAME native_mcc_ctf_assault_complete_matches_reference_tu15 COMMAND ctf_assault_match_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-complete-matches-reference.json")
        set_tests_properties(native_mcc_ctf_assault_complete_matches_reference_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 3600 ENVIRONMENT "HALO2_REFERENCE_OBJECTIVE_NATIVE_KERNELS=1")
        add_test(NAME native_mcc_ctf_assault_complete_matches_comparison_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/compare_objective_native_matches_tu15.py"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-complete-matches.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-complete-matches-reference.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-complete-matches-comparison.json")
        set_tests_properties(native_mcc_ctf_assault_complete_matches_comparison_tu15 PROPERTIES
            DEPENDS "native_mcc_ctf_assault_complete_matches_tu15;native_mcc_ctf_assault_complete_matches_reference_tu15")
        add_test(NAME native_mcc_assault_multi_bomb_complete_match_tu15 COMMAND ctf_assault_match_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/assault-multi-bomb-complete-match.json")
        set_tests_properties(native_mcc_assault_multi_bomb_complete_match_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 1200 ENVIRONMENT "HALO2_OBJECTIVE_VARIANT=Multi Bomb")
        add_test(NAME native_mcc_multiplayer_objective_flow_tu15 COMMAND multiplayer_objective_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-objective-flow.json")
        set_tests_properties(native_mcc_multiplayer_objective_flow_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800)
        add_test(NAME native_mcc_multiplayer_objective_goals_tu15 COMMAND multiplayer_objective_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-objective-goals.json")
        set_tests_properties(native_mcc_multiplayer_objective_goals_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 1800 ENVIRONMENT "HALO2_OBJECTIVE_GOAL_FLOW=1")
        add_test(NAME native_mcc_multiplayer_objective_wins_tu15 COMMAND multiplayer_objective_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-objective-wins.json")
        set_tests_properties(native_mcc_multiplayer_objective_wins_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_OBJECTIVE_WIN_FLOW=1")
        add_test(NAME native_mcc_multiplayer_objective_overtime_tu15 COMMAND multiplayer_objective_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-objective-overtime.json")
        set_tests_properties(native_mcc_multiplayer_objective_overtime_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_OBJECTIVE_OVERTIME_FLOW=1")
        add_test(NAME native_mcc_multiplayer_objective_overtime_disabled_tu15 COMMAND multiplayer_objective_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-objective-overtime-disabled.json")
        set_tests_properties(native_mcc_multiplayer_objective_overtime_disabled_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_OBJECTIVE_OVERTIME_FLOW=1;HALO2_OBJECTIVE_OVERTIME_DISABLED=1")
        add_test(NAME native_mcc_ctf_assault_returns_tu15 COMMAND multiplayer_objective_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/ctf-assault-returns.json")
        set_tests_properties(native_mcc_ctf_assault_returns_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 2400 ENVIRONMENT "HALO2_OBJECTIVE_RETURN_FLOW=1")
        add_test(NAME native_mcc_assault_arming_decay_tu15 COMMAND multiplayer_objective_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/assault-arming-decay.json")
        set_tests_properties(native_mcc_assault_arming_decay_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 1800 ENVIRONMENT "HALO2_OBJECTIVE_ARMING_DECAY=1")
        add_test(NAME native_mcc_assault_arming_decay_reference_tu15 COMMAND multiplayer_objective_flow_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/assault-arming-decay-reference.json")
        set_tests_properties(native_mcc_assault_arming_decay_reference_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 1800 ENVIRONMENT "HALO2_OBJECTIVE_ARMING_DECAY=1;HALO2_REFERENCE_OBJECTIVE_NATIVE_KERNELS=1")
        add_executable(multiplayer_engine_frame_events_tu15_tests tests/multiplayer_engine_frame_events_tu15_tests.cpp)
        target_include_directories(multiplayer_engine_frame_events_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(multiplayer_engine_frame_events_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_mcc_multiplayer_frame_events_tu15 COMMAND multiplayer_engine_frame_events_tu15_tests
            "${HALO2_MCC_TELEPORTER_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engine-frame-events.json")
        set_tests_properties(native_mcc_multiplayer_frame_events_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
        add_test(NAME native_mcc_multiplayer_prepared_engines_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-prepared-engines.json")
        set_tests_properties(native_mcc_multiplayer_prepared_engines_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 1200 ENVIRONMENT "HALO2_ENGINE_PREPARED_MATCH=1")
        add_test(NAME native_mcc_multiplayer_prepared_finalization_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-prepared-finalization.json")
        set_tests_properties(native_mcc_multiplayer_prepared_finalization_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 1800
            ENVIRONMENT "HALO2_ENGINE_PREPARED_MATCH=1;HALO2_ENGINE_PREPARED_FINALIZATION=1")
        add_test(NAME native_mcc_multiplayer_prepared_slayer_finalization_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-prepared-slayer-finalization.json")
        set_tests_properties(native_mcc_multiplayer_prepared_slayer_finalization_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180
            ENVIRONMENT "HALO2_ENGINE_PREPARED_MATCH=1;HALO2_ENGINE_PREPARED_FINALIZATION=1;HALO2_ENGINE_WORLD_VARIANT=Slayer")
        add_test(NAME native_mcc_multiplayer_prepared_time_finalization_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-prepared-time-finalization.json")
        set_tests_properties(native_mcc_multiplayer_prepared_time_finalization_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180
            ENVIRONMENT "HALO2_ENGINE_PREPARED_MATCH=1;HALO2_ENGINE_PREPARED_FINALIZATION=1;HALO2_ENGINE_WORLD_VARIANT=King")
        add_test(NAME native_mcc_multiplayer_prepared_views_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-prepared-views.json")
        set_tests_properties(native_mcc_multiplayer_prepared_views_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180
            ENVIRONMENT "HALO2_ENGINE_PREPARED_MATCH=1;HALO2_ENGINE_PREPARED_VIEWS=1;HALO2_ENGINE_WORLD_VARIANT=Slayer")
        foreach(mode IN ITEMS ffa team)
            add_test(NAME native_mcc_multiplayer_prepared_${mode}_cap_tu15 COMMAND multiplayer_engines_world_tu15_tests
                "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
                "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-prepared-${mode}-cap.json")
            set(cap_variant "Slayer")
            if(mode STREQUAL "team")
                set(cap_variant "Team Slayer")
            endif()
            set_tests_properties(native_mcc_multiplayer_prepared_${mode}_cap_tu15 PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 180
                ENVIRONMENT "HALO2_ENGINE_PREPARED_MATCH=1;HALO2_ENGINE_PREPARED_ACTIVE_LIMIT=1;HALO2_ENGINE_WORLD_VARIANT=${cap_variant}")
        endforeach()
        add_test(NAME native_mcc_multiplayer_engines_occupied_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engines-occupied-world.json")
        set_tests_properties(native_mcc_multiplayer_engines_occupied_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 600 ENVIRONMENT "HALO2_ENGINE_WORLD_OCCUPIED=1")
        add_test(NAME native_mcc_multiplayer_engines_spawn_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engines-spawn-world.json")
        set_tests_properties(native_mcc_multiplayer_engines_spawn_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 600
            ENVIRONMENT "HALO2_ENGINE_WORLD_OCCUPIED=1;HALO2_ENGINE_WORLD_SPAWN=1")
        add_test(NAME native_mcc_multiplayer_engines_world_tick_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engines-world-tick.json")
        set_tests_properties(native_mcc_multiplayer_engines_world_tick_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 900
            ENVIRONMENT "HALO2_ENGINE_WORLD_OCCUPIED=1;HALO2_ENGINE_WORLD_SPAWN=1;HALO2_ENGINE_WORLD_TICK=1")
        add_test(NAME native_mcc_multiplayer_match_adapter_tick_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-match-adapter-tick.json")
        set_tests_properties(native_mcc_multiplayer_match_adapter_tick_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180 ENVIRONMENT "HALO2_ENGINE_MATCH_ADAPTER_TICK=1")
        foreach(ending IN ITEMS player team tie)
            add_test(NAME native_mcc_multiplayer_${ending}_ending_tu15 COMMAND multiplayer_engines_world_tu15_tests
                "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
                "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-${ending}-ending.json")
            set(ending_environment "HALO2_ENGINE_MATCH_ADAPTER_TICK=1;HALO2_ENGINE_MATCH_GAME_OVER=1")
            if(ending STREQUAL "team")
                string(APPEND ending_environment ";HALO2_ENGINE_MATCH_TEAM_PLAYERS=1")
            elseif(ending STREQUAL "tie")
                string(APPEND ending_environment ";HALO2_ENGINE_MATCH_TIED_END=1")
            endif()
            set_tests_properties(native_mcc_multiplayer_${ending}_ending_tu15 PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 240 ENVIRONMENT "${ending_environment}")
        endforeach()
        add_test(NAME native_mcc_multiplayer_player_factory_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-player-factory.json")
        set_tests_properties(native_mcc_multiplayer_player_factory_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180
            ENVIRONMENT "HALO2_ENGINE_MATCH_ADAPTER_TICK=1;HALO2_ENGINE_MATCH_PLAYER_FACTORY_FLOW=1")
        add_test(NAME native_mcc_multiplayer_player_factory_lives_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-player-factory-lives.json")
        set_tests_properties(native_mcc_multiplayer_player_factory_lives_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180
            ENVIRONMENT "HALO2_ENGINE_MATCH_ADAPTER_TICK=1;HALO2_ENGINE_MATCH_PLAYER_FACTORY_FLOW=1;HALO2_ENGINE_MATCH_FINITE_LIVES=1")
        add_test(NAME native_mcc_multiplayer_team_player_factory_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-team-player-factory.json")
        set_tests_properties(native_mcc_multiplayer_team_player_factory_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180
            ENVIRONMENT "HALO2_ENGINE_MATCH_ADAPTER_TICK=1;HALO2_ENGINE_MATCH_PLAYER_FACTORY_FLOW=1;HALO2_ENGINE_MATCH_FINITE_LIVES=1;HALO2_ENGINE_MATCH_TEAM_PLAYERS=1")
        add_test(NAME native_mcc_multiplayer_declared_player_placement_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-declared-player-placement.json")
        set_tests_properties(native_mcc_multiplayer_declared_player_placement_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180
            ENVIRONMENT "HALO2_ENGINE_MATCH_ADAPTER_TICK=1;HALO2_ENGINE_MATCH_DECLARED_PLACEMENT=1")
        add_test(NAME native_mcc_multiplayer_engines_rounds_tu15 COMMAND multiplayer_engines_world_tu15_tests
            "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json"
            "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-engines-rounds-world.json")
        set_tests_properties(native_mcc_multiplayer_engines_rounds_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 600
            ENVIRONMENT "HALO2_ENGINE_WORLD_OCCUPIED=1;HALO2_ENGINE_WORLD_ROUNDS=1")
    endif()
    add_executable(native_tu15_multiplayer_data_tests
        tests/native_tu15_multiplayer_data_tests.cpp
        src/gameplay/multiplayer/native_tu15_multiplayer_data.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_multiplayer_data_tests PRIVATE include)
    target_link_libraries(native_tu15_multiplayer_data_tests PRIVATE
        halo2_snapshot_storage Threads::Threads)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_multiplayer_data_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_multiplayer_data_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_multiplayer_data COMMAND native_tu15_multiplayer_data_tests)
    add_executable(multiplayer_engines_tu15_tests
        tests/multiplayer_engines_tu15_tests.cpp
        tests/multiplayer_engine_services_tu15_generated.cpp
        src/gameplay/multiplayer/source_game_variant.cpp
        tests/ctf_activation_test_support_generated.cpp
        src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_player_iterator_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_activation_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_activation_spawn_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_configuration_tu15_generated.cpp)
    target_include_directories(multiplayer_engines_tu15_tests PRIVATE
        include src tests ${HALO2_TEST_JSON_INCLUDE})
    target_link_libraries(multiplayer_engines_tu15_tests PRIVATE OpenSSL::Crypto)
    add_test(NAME original_ctf_assault_native_callbacks_tu15 COMMAND multiplayer_engines_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/objective_callbacks_original_tu15.json")
    if(MSVC)
        target_compile_options(multiplayer_engines_tu15_tests PRIVATE /fp:strict)
    else()
        target_compile_options(multiplayer_engines_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(multiplayer_engines_tu15_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(multiplayer_engines_tu15_tests PRIVATE
            -fsanitize=address,undefined)
    endif()
    add_test(NAME original_multiplayer_engines_tu15 COMMAND multiplayer_engines_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/multiplayer_engines_original_tu15.json")
endif()
