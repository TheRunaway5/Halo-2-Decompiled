# DVD payloads are offline reference controls only. Production uses admitted
# MCC high-definition UI/audio and optional missing-asset Mod Kit imports.
if(BUILD_TESTING)
    add_executable(original_matchmaking_font_tests tests/original_matchmaking_fonts_tests.cpp
        src/ui/original_matchmaking_fonts.cpp)
    target_link_libraries(original_matchmaking_font_tests PRIVATE halo2_cache)
    add_test(NAME original_matchmaking_font_payload COMMAND original_matchmaking_font_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/fonts")
    set_tests_properties(original_matchmaking_font_payload PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    # The same independent original-cache reader also checks exact private
    # ADPCM/PCM, real mixer output, source class and natural voice completion.
    target_sources(frontend_sound_slots_tests PRIVATE src/ui/original_matchmaking_audio.cpp)
    add_test(NAME original_matchmaking_audio_payload COMMAND frontend_sound_slots_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/halo2.exe")
    set_tests_properties(original_matchmaking_audio_payload PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(original_matchmaking_emblems_tests tests/original_matchmaking_emblems_tests.cpp
        src/ui/original_matchmaking_emblems.cpp)
    target_link_libraries(original_matchmaking_emblems_tests PRIVATE halo2_cache)
    if(HALO2_MCC_PARTY_UI_TEST_CACHE)
        add_test(NAME original_matchmaking_emblem_reference COMMAND original_matchmaking_emblems_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map"
            "${HALO2_MCC_PARTY_UI_TEST_CACHE}/halo2/h2_maps_win64_dx11/mainmenu.map")
        set_tests_properties(original_matchmaking_emblem_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    endif()
endif()
