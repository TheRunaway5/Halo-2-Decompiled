if(BUILD_TESTING)
    add_executable(full_scoreboard_kerning_tu15_tests tests/full_scoreboard_kerning_tu15_tests.cpp)
    target_link_libraries(full_scoreboard_kerning_tu15_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_full_scoreboard_kerning_tu15 COMMAND full_scoreboard_kerning_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scoreboard_text_kerning_tu15_original.json")
endif()
