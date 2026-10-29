if(BUILD_TESTING)
    add_executable(source_scene_handoff_tests tests/source_scene_handoff_tests.cpp)
    target_link_libraries(source_scene_handoff_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME source_scene_lifetime_handoff COMMAND ${CMAKE_COMMAND} -E env
        HALO2_REFERENCE_DYNAMIC_OBJECT_POSE_CACHE=1 $<TARGET_FILE:source_scene_handoff_tests>
        "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/zanzibar.map"
        "${CMAKE_CURRENT_BINARY_DIR}/source-scene-handoff.json")
    set_tests_properties(source_scene_lifetime_handoff PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME source_scene_lifetime_handoff_dynamic_pose_cache COMMAND ${CMAKE_COMMAND} -E env
        HALO2_NATIVE_DYNAMIC_OBJECT_POSE_CACHE=1 HALO2_REFERENCE_DYNAMIC_OBJECT_POSE_CACHE=
        HALO2_DYNAMIC_OBJECT_POSE_DIAGNOSTICS=1 $<TARGET_FILE:source_scene_handoff_tests>
        "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/zanzibar.map"
        "${CMAKE_CURRENT_BINARY_DIR}/source-scene-handoff-dynamic-pose-cache.json")
    set_tests_properties(source_scene_lifetime_handoff_dynamic_pose_cache PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
