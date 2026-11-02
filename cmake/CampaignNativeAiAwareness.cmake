include_guard(GLOBAL)
target_sources(halo2_systems PRIVATE src/campaign/ai/native_campaign_ai_awareness_tu15.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/campaign/ai/native_campaign_ai_awareness_tu15.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86"
   AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_executable(native_campaign_ai_awareness_tu15_tests
        tests/native_campaign_ai_awareness_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_awareness_tu15.cpp
        src/campaign/ai/native_campaign_ai_query_tu15.cpp
        src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp)
    target_include_directories(native_campaign_ai_awareness_tu15_tests PRIVATE include)
    target_compile_features(native_campaign_ai_awareness_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_ai_awareness_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_campaign_ai_awareness_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME native_campaign_ai_awareness_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_awareness_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_awareness_tu15.json.gz")
    set_tests_properties(native_campaign_ai_awareness_tu15 PROPERTIES TIMEOUT 120)
endif()
