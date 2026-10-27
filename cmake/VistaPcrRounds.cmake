if(BUILD_TESTING)
 add_executable(vista_pcr_round_count_tests tests/vista_pcr_round_count_tests.cpp
  src/ui/vista/vista_pcr_publication.cpp src/gameplay/multiplayer/source_game_variant.cpp)
 target_include_directories(vista_pcr_round_count_tests PRIVATE include src)
 target_link_libraries(vista_pcr_round_count_tests PRIVATE nlohmann_json::nlohmann_json)
 add_test(NAME original_vista_pcr_round_count COMMAND vista_pcr_round_count_tests
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_round_count_original.json")
 set_tests_properties(original_vista_pcr_round_count PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
endif()
