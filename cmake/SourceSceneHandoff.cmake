if(BUILD_TESTING)
    add_executable(source_scene_handoff_tests tests/source_scene_handoff_tests.cpp)
    target_link_libraries(source_scene_handoff_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME source_scene_lifetime_handoff COMMAND source_scene_handoff_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map"
        "${CMAKE_CURRENT_BINARY_DIR}/source-scene-handoff.json")
    set_tests_properties(source_scene_lifetime_handoff PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
