if(BUILD_TESTING)
    add_executable(multiplayer_hud_continuity_tests tests/multiplayer_hud_continuity_tests.cpp)
    target_link_libraries(multiplayer_hud_continuity_tests PRIVATE halo2_renderer)
    add_test(NAME native_hud_scoreboard_continuity COMMAND multiplayer_hud_continuity_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    set_tests_properties(native_hud_scoreboard_continuity PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-hud/hud.tsv")
        add_test(NAME native_hud_scoreboard_continuity_mcc COMMAND multiplayer_hud_continuity_tests
            "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-hud")
        set_tests_properties(native_hud_scoreboard_continuity_mcc PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    endif()
endif()
