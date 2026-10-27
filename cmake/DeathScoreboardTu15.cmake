target_sources(halo2_systems PRIVATE src/ui/hud/source_player_status.cpp)

if(BUILD_TESTING)
    set(HALO2_DEATH_SCOREBOARD_XBE
        "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        CACHE FILEPATH "Original TU1.5 reference for death-screen and scoreboard checks")
    set(HALO2_DEATH_SCOREBOARD_MAPS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        CACHE PATH "Owned maps for native scoreboard delivery checks")

    add_executable(multiplayer_scoreboard_modes_tests tests/multiplayer_scoreboard_modes_tests.cpp)
    target_link_libraries(multiplayer_scoreboard_modes_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_multiplayer_scoreboard_modes COMMAND multiplayer_scoreboard_modes_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/team_full_scoreboard_original.json)

    add_executable(multiplayer_scoreboard_modes_native_tests tests/multiplayer_scoreboard_modes_native_tests.cpp)
    target_link_libraries(multiplayer_scoreboard_modes_native_tests PRIVATE halo2_systems)
    add_test(NAME native_multiplayer_scoreboard_modes COMMAND multiplayer_scoreboard_modes_native_tests
        ${HALO2_DEATH_SCOREBOARD_MAPS}/lockout.map ${HALO2_DEATH_SCOREBOARD_XBE})
    set_tests_properties(native_multiplayer_scoreboard_modes PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    if(HALO2_MCC_ONLINE_PRESENTATION_MAP)
        add_test(NAME native_mcc_scoreboard_all39_presets COMMAND multiplayer_scoreboard_modes_native_tests
            "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" native-mcc)
        set_tests_properties(native_mcc_scoreboard_all39_presets PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
            RUN_SERIAL TRUE LABELS "online-presentation;mcc;native-component")
    endif()

    add_executable(source_player_status_tests tests/source_player_status_tests.cpp)
    target_link_libraries(source_player_status_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_player_status COMMAND source_player_status_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_status_tu15_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map ${HALO2_DEATH_SCOREBOARD_XBE})
    set_tests_properties(original_tu15_player_status PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)

    add_executable(multiplayer_scoreboard_phase_tu15_tests tests/multiplayer_scoreboard_phase_tu15_tests.cpp)
    target_link_libraries(multiplayer_scoreboard_phase_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_multiplayer_scoreboard_phase_tu15 COMMAND multiplayer_scoreboard_phase_tu15_tests
        ${HALO2_DEATH_SCOREBOARD_MAPS}/lockout.map ${HALO2_DEATH_SCOREBOARD_XBE})
    set_tests_properties(native_multiplayer_scoreboard_phase_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)

    add_executable(multiplayer_scoreboard_controller_runtime_tests
        tests/multiplayer_scoreboard_controller_runtime_tests.cpp
        src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(multiplayer_scoreboard_controller_runtime_tests
        PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
    target_compile_definitions(multiplayer_scoreboard_controller_runtime_tests
        PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
    add_dependencies(multiplayer_scoreboard_controller_runtime_tests halo2_shaders)
    add_test(NAME native_multiplayer_scoreboard_controllers COMMAND multiplayer_scoreboard_controller_runtime_tests
        ${HALO2_DEATH_SCOREBOARD_MAPS}/lockout.map ${HALO2_DEATH_SCOREBOARD_MAPS} ${HALO2_DEATH_SCOREBOARD_XBE})
    set_tests_properties(native_multiplayer_scoreboard_controllers PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
