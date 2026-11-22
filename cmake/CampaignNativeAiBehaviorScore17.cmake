include_guard(GLOBAL)
target_sources(halo2_physics_world_runtime PRIVATE
    src/campaign/ai/native_campaign_ai_behavior_score_17_tu15.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_campaign_ai_behavior_score_17_tu15_tests
        tests/native_campaign_ai_behavior_score_17_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_behavior_score_17_tu15.cpp)
    target_include_directories(native_campaign_ai_behavior_score_17_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_ai_behavior_score_17_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_ai_behavior_score_17_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    add_test(NAME native_campaign_ai_behavior_score_17_tu15
        COMMAND native_campaign_ai_behavior_score_17_tu15_tests)
endif()
