target_sources(halo2_physics_world_runtime PRIVATE
    src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp
    src/gameplay/multiplayer/source_result_recording_support_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/native_tu15_multiplayer_data.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/native_tu15_multiplayer_branch_data.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_multiplayer_engine_entity_tu15.cpp)
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
    add_executable(result_recording_support_tu15_tests tests/result_recording_support_tu15_tests.cpp)
    target_include_directories(result_recording_support_tu15_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
    target_link_libraries(result_recording_support_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_result_recording_support_tu15 COMMAND result_recording_support_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/result_recording_support_original_tu15.json")
    set(HALO2_MCC_MULTIPLAYER_ENGINE_MAP "" CACHE FILEPATH
        "Imported MCC map for actual TU1.5 multiplayer engine integration")
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
        src/gameplay/multiplayer/source_ctf_configuration_tu15_generated.cpp)
    target_include_directories(multiplayer_engines_tu15_tests PRIVATE
        include src tests ${HALO2_TEST_JSON_INCLUDE})
    target_link_libraries(multiplayer_engines_tu15_tests PRIVATE OpenSSL::Crypto)
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
