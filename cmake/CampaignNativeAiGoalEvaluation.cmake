include_guard(GLOBAL)
# Native 26A380 remainder (26A712..26AF82) and its scoring callees: the
# runtime sources are registered with the 1BBEE0 owner in
# CampaignNativeAiAreaPerform.cmake. This module adds their isolated
# physical-original replay.
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_campaign_ai_goal_evaluation_tu15_tests
        tests/native_campaign_ai_goal_evaluation_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_goal_evaluation_tu15.cpp
        src/campaign/ai/native_campaign_ai_goal_scoring_tu15.cpp
        src/campaign/ai/native_campaign_ai_area_perform_tu15.cpp
        src/campaign/ai/native_campaign_ai_navigation_filter_tu15.cpp
        src/campaign/ai/native_campaign_ai_character_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_workspace_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_search_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_query_start_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_heap_tu15.cpp
        src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp
        src/campaign/ai/native_campaign_ai_frame_geometry_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_influence_tu15.cpp
        src/campaign/ai/native_campaign_ai_jump_arc_tu15.cpp
        src/campaign/ai/native_campaign_ai_jump_fallback_tu15.cpp
        src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_result_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_candidates_tu15.cpp
        src/campaign/ai/native_campaign_object_world_point_tu15.cpp)
    # The 1BBEE0 owner (for the refusal type) links its full closure.
    get_target_property(_halo2_goal_evaluation_sources native_campaign_ai_area_perform_tu15_tests SOURCES)
    list(FILTER _halo2_goal_evaluation_sources EXCLUDE REGEX "^tests/")
    target_sources(native_campaign_ai_goal_evaluation_tu15_tests PRIVATE ${_halo2_goal_evaluation_sources})
    get_target_property(_halo2_goal_evaluation_libraries native_campaign_ai_area_perform_tu15_tests LINK_LIBRARIES)
    target_link_libraries(native_campaign_ai_goal_evaluation_tu15_tests PRIVATE ${_halo2_goal_evaluation_libraries})
    target_include_directories(native_campaign_ai_goal_evaluation_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_ai_goal_evaluation_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_ai_goal_evaluation_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_goal_evaluation_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_goal_evaluation_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    # Physical unchanged-i386 reference: tools/oracle_native_campaign_ai_goal_evaluation_tu15.py
    add_test(NAME native_campaign_ai_goal_evaluation_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_goal_evaluation_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_goal_evaluation_tu15.json.gz")
    set_tests_properties(native_campaign_ai_goal_evaluation_tu15 PROPERTIES TIMEOUT 300)
endif()
