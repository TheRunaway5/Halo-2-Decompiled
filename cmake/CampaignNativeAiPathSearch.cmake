include_guard(GLOBAL)
# Native 27B2B0 path search (26A380's CALL at 26A831) with its 27B340
# expansion, 27BD30 neighbor rows and 202970/186E30 device gate. It composes
# the native heap, 27B100 query-start, point/frame geometry, 27C520
# influence and 278E60 jump-link owners registered by the modules above.
target_sources(halo2_physics_world_runtime PRIVATE
    src/campaign/ai/native_campaign_ai_path_search_tu15.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_campaign_ai_path_search_tu15_tests
        tests/native_campaign_ai_path_search_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_path_search_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_query_start_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_heap_tu15.cpp
        src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp
        src/campaign/ai/native_campaign_ai_frame_geometry_tu15.cpp
        src/campaign/ai/native_campaign_ai_path_influence_tu15.cpp
        src/campaign/ai/native_campaign_ai_jump_arc_tu15.cpp
        src/campaign/ai/native_campaign_ai_jump_fallback_tu15.cpp
        src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
    target_include_directories(native_campaign_ai_path_search_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_ai_path_search_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_ai_path_search_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_campaign_ai_path_search_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_path_search_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_path_search_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    # Physical unchanged-i386 reference: tools/oracle_native_campaign_ai_path_search_tu15.py
    add_test(NAME native_campaign_ai_path_search_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_path_search_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_path_search_tu15.json.gz")
    set_tests_properties(native_campaign_ai_path_search_tu15 PROPERTIES TIMEOUT 300)
endif()
