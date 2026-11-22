include_guard(GLOBAL)
# Native 26C3C0 (1BBEE0's goal publication) with 1FC540/1FC9B0/200D30 movement
# and its small leaves. Composes the 26C510/26C600/26C850/16E940, character
# property, point geometry, navigation filter, path workspace/arena and
# 27B2B0 path-search owners registered by the modules above.
target_sources(halo2_physics_world_runtime PRIVATE
    src/campaign/ai/native_campaign_ai_publication_tu15.cpp
    src/campaign/ai/native_campaign_ai_movement_resume_tu15.cpp
    src/campaign/ai/native_campaign_ai_vehicle_movement_candidates_tu15.cpp
    src/campaign/ai/native_campaign_ai_vehicle_movement_score_tu15.cpp
    src/campaign/ai/native_campaign_ai_vehicle_steering_tu15.cpp
    src/campaign/ai/native_campaign_ai_vehicle_movement_scale_tu15.cpp
    src/campaign/ai/native_campaign_ai_vehicle_movement_driver_tu15.cpp
    src/campaign/ai/native_campaign_ai_route_tu15.cpp
    src/campaign/ai/native_campaign_ai_route_special_link_tu15.cpp
    src/campaign/ai/native_campaign_ai_route_segments_tu15.cpp
    src/campaign/ai/native_campaign_ai_route_traversal_tu15.cpp
    src/campaign/ai/native_campaign_ai_route_smoothing_tu15.cpp
    src/campaign/ai/native_campaign_ai_route_corridor_tu15.cpp
    src/campaign/ai/native_campaign_ai_route_obstacles_tu15.cpp
    src/campaign/ai/native_campaign_ai_route_obstacle_geometry_tu15.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_campaign_ai_vehicle_movement_scale_tu15_tests
        tests/native_campaign_ai_vehicle_movement_scale_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_vehicle_movement_scale_tu15.cpp
        src/campaign/ai/native_campaign_ai_character_property_tu15.cpp
        src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_campaign_ai_vehicle_movement_scale_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_ai_vehicle_movement_scale_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(native_campaign_ai_vehicle_movement_scale_tu15_tests PRIVATE
        halo2_snapshot_storage Threads::Threads)
    target_compile_options(native_campaign_ai_vehicle_movement_scale_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_vehicle_movement_scale_tu15_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_vehicle_movement_scale_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_campaign_ai_vehicle_movement_scale_tu15
        COMMAND native_campaign_ai_vehicle_movement_scale_tu15_tests)
    set_tests_properties(native_campaign_ai_vehicle_movement_scale_tu15 PROPERTIES TIMEOUT 60)

    # Sources the publication closure adds to any 1BBEE0 test target.
    set(HALO2_CAMPAIGN_AI_PUBLICATION_SOURCES
        src/campaign/ai/native_campaign_ai_publication_tu15.cpp
        src/campaign/ai/native_campaign_ai_movement_resume_tu15.cpp
        src/campaign/ai/native_campaign_ai_route_tu15.cpp
        src/campaign/ai/native_campaign_ai_route_special_link_tu15.cpp
        src/campaign/ai/native_campaign_ai_route_segments_tu15.cpp
        src/campaign/ai/native_campaign_ai_route_traversal_tu15.cpp
        src/campaign/ai/native_campaign_ai_route_smoothing_tu15.cpp
        src/campaign/ai/native_campaign_ai_route_corridor_tu15.cpp
        src/campaign/ai/native_campaign_ai_route_obstacles_tu15.cpp
        src/campaign/ai/native_campaign_ai_route_obstacle_geometry_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_candidates_tu15.cpp
        src/campaign/ai/native_campaign_ai_navigation_filter_tu15.cpp
        src/campaign/ai/native_campaign_ai_character_tu15.cpp
        src/campaign/ai/native_campaign_ai_orders_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_workspace_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_search_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_query_start_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_heap_tu15.cpp
        src/campaign/ai/native_campaign_ai_frame_geometry_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_influence_tu15.cpp
        src/campaign/ai/native_campaign_ai_jump_arc_tu15.cpp
        src/campaign/ai/native_campaign_ai_jump_fallback_tu15.cpp
        src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
    # The 1BBEE0 owner calls native 26C3C0, so its test links this closure.
    target_sources(native_campaign_ai_area_perform_tu15_tests PRIVATE ${HALO2_CAMPAIGN_AI_PUBLICATION_SOURCES})
    target_link_libraries(native_campaign_ai_area_perform_tu15_tests PRIVATE
        halo2_native_tu15_data_array_lookup halo2_systems halo2_native_weapon_effect_dispatch_tu15)
    # The publication replay reuses that target's (deduplicated) sources.
    get_target_property(_halo2_publication_sources native_campaign_ai_area_perform_tu15_tests SOURCES)
    list(FILTER _halo2_publication_sources EXCLUDE REGEX "^tests/")
    list(REMOVE_DUPLICATES _halo2_publication_sources)
    add_executable(native_campaign_ai_publication_tu15_tests
        tests/native_campaign_ai_publication_tu15_tests.cpp ${_halo2_publication_sources})
    target_include_directories(native_campaign_ai_publication_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_ai_publication_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_ai_publication_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_campaign_ai_publication_tu15_tests PRIVATE nlohmann_json::nlohmann_json
        halo2_native_tu15_data_array_delete halo2_native_tu15_data_array_lookup halo2_native_pair_consumer
        halo2_systems halo2_native_weapon_effect_dispatch_tu15)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_publication_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_publication_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    # Physical unchanged-i386 reference: tools/oracle_native_campaign_ai_publication_tu15.py
    add_test(NAME native_campaign_ai_publication_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_publication_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_publication_tu15.json.gz")
    set_tests_properties(native_campaign_ai_publication_tu15 PROPERTIES TIMEOUT 300)
endif()
