# Typed campaign AI queries borrow the retained world and never dispatch lifts.
target_sources(halo2_systems PRIVATE src/campaign/ai/native_campaign_ai_query_tu15.cpp
    src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp)
if(BUILD_TESTING)
    add_executable(native_campaign_ai_query_tu15_tests
        tests/native_campaign_ai_query_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_query_tu15.cpp
        src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp)
    target_include_directories(native_campaign_ai_query_tu15_tests PRIVATE include)
    target_link_libraries(native_campaign_ai_query_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_native_campaign_ai_query_tu15
        COMMAND native_campaign_ai_query_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_living_count_tu15_v3.json")
    add_test(NAME original_native_campaign_ai_count_family_tu15
        COMMAND native_campaign_ai_query_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_count_family_tu15.json")
    add_test(NAME original_native_campaign_ai_fighting_count_tu15
        COMMAND native_campaign_ai_query_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_fighting_count_tu15.json")
    add_test(NAME original_native_campaign_ai_living_fraction_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_query_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_living_fraction_tu15.json.gz")
endif()
if(TARGET halo2_cache)
    include(cmake/CampaignNativeAiFractionLayout.cmake)
endif()
