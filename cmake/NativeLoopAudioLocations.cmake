include_guard(GLOBAL)

if(BUILD_TESTING AND TARGET halo2_systems)
    add_executable(loop_audio_locations_tu15_tests EXCLUDE_FROM_ALL
        tests/loop_audio_locations_tu15_tests.cpp)
    target_link_libraries(loop_audio_locations_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_loop_audio_locations COMMAND loop_audio_locations_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/loop_audio_locations_tu15_original.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/loop_audio_locations_tu15_marker_physical.json"
        --report-output "${CMAKE_CURRENT_BINARY_DIR}/loop-audio-locations.json")
    set_tests_properties(original_tu15_loop_audio_locations PROPERTIES
        TIMEOUT 90 LABELS "original;audio;effect;native-owner")
endif()
