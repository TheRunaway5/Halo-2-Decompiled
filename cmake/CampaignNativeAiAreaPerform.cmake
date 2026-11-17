include_guard(GLOBAL)
# Native 1BBEE0 (behavior row 109 perform), its 26A2E0/26B220/26BA30 goal
# selection with the 26A380 prefix, and 1BC0A0 (row 109 +48) owners.
# Requires CampaignNativeAiAreaChoiceBinding/PathArena/ObjectMarkerRequest
# sources, which CMakeLists.txt includes before this module.
target_sources(halo2_physics_world_runtime PRIVATE
    src/campaign/ai/native_campaign_ai_area_perform_tu15.cpp
    src/campaign/ai/native_campaign_ai_actor_path_query_tu15.cpp
    src/campaign/ai/native_campaign_ai_object_navigation_tu15.cpp
    src/campaign/ai/native_campaign_ai_biped_refresh_tu15.cpp
    src/campaign/ai/native_campaign_ai_navigation_lookup_tu15.cpp
    src/campaign/ai/native_campaign_ai_area_perform_tu15_binding.cpp
    src/campaign/ai/native_campaign_ai_candidate_collection_tu15.cpp
    src/campaign/ai/native_campaign_ai_goal_evaluation_tu15.cpp
    src/campaign/ai/native_campaign_ai_goal_scoring_tu15.cpp
    src/campaign/ai/native_campaign_ai_character_property_tu15.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_campaign_ai_area_perform_tu15_tests
        tests/native_campaign_ai_area_perform_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_area_perform_tu15.cpp
        src/campaign/ai/native_campaign_ai_actor_path_query_tu15.cpp
        src/campaign/ai/native_campaign_ai_object_navigation_tu15.cpp
        src/campaign/ai/native_campaign_ai_biped_refresh_tu15.cpp
        src/campaign/ai/native_campaign_ai_navigation_lookup_tu15.cpp
        src/campaign/ai/native_campaign_ai_candidate_collection_tu15.cpp
        src/campaign/ai/native_campaign_ai_goal_evaluation_tu15.cpp
        # 26A380 remainder: scoring callees, 201500, 27B010, 27B2B0, 27A460.
        src/campaign/ai/native_campaign_ai_goal_scoring_tu15.cpp
        src/campaign/ai/native_campaign_ai_navigation_filter_tu15.cpp
        src/campaign/ai/native_campaign_ai_character_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_workspace_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_search_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_query_start_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_heap_tu15.cpp
        src/campaign/ai/native_campaign_ai_frame_geometry_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_influence_tu15.cpp
        src/campaign/ai/native_campaign_ai_jump_arc_tu15.cpp
        src/campaign/ai/native_campaign_ai_jump_fallback_tu15.cpp
        src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_result_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_candidates_tu15.cpp
        src/campaign/ai/native_campaign_object_world_point_tu15.cpp
        src/campaign/ai/native_campaign_ai_character_property_tu15.cpp
        src/campaign/ai/native_campaign_ai_area_choice_tu15.cpp
        src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_distance_tu15.cpp
        src/campaign/ai/native_campaign_ai_action_identity_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_arena_tu15.cpp
        src/campaign/ai/native_campaign_ai_resource_retirement_tu15.cpp
        src/campaign/ai/native_campaign_object_marker_request_tu15.cpp
        src/campaign/ai/native_campaign_object_markers_tu15.cpp
        src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp)
    target_include_directories(native_campaign_ai_area_perform_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_ai_area_perform_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_ai_area_perform_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_campaign_ai_area_perform_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_native_tu15_data_array_delete halo2_native_pair_consumer)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_area_perform_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_area_perform_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    # Physical unchanged-i386 reference: tools/oracle_native_campaign_ai_area_perform_tu15.py
    add_test(NAME native_campaign_ai_area_perform_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_area_perform_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_area_perform_tu15.json.gz")
    set_tests_properties(native_campaign_ai_area_perform_tu15 PROPERTIES TIMEOUT 120)
    add_test(NAME native_campaign_ai_character_object_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_area_perform_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_character_object_tu15.json.gz")
    set_tests_properties(native_campaign_ai_character_object_tu15 PROPERTIES TIMEOUT 120)
    add_test(NAME native_campaign_ai_actor_path_query_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_area_perform_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_actor_path_query_tu15.json.gz")
    set_tests_properties(native_campaign_ai_actor_path_query_tu15 PROPERTIES TIMEOUT 120)
    add_test(NAME native_campaign_ai_object_navigation_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_area_perform_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_object_navigation_tu15.json.gz")
    set_tests_properties(native_campaign_ai_object_navigation_tu15 PROPERTIES TIMEOUT 120)
endif()
