if(BUILD_TESTING)
    add_executable(source_render_visibility_row_tu15_tests
        tests/source_render_visibility_row_tu15_tests.cpp)
    target_link_libraries(source_render_visibility_row_tu15_tests PRIVATE
        halo2_physics_world_runtime nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_render_visibility_row
        COMMAND source_render_visibility_row_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_render_visibility_row_tu15.jsonl)
endif()
