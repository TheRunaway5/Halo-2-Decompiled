include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/CampaignNativeAiPerceptionGroupLink.cmake")
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/native_campaign_ai_perception_lookup_tu15.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_campaign_ai_perception_lookup_tu15_tests
        tests/native_campaign_ai_perception_lookup_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_perception_lookup_tu15.cpp
        src/campaign/ai/native_campaign_ai_object_actor_tu15.cpp
        src/campaign/ai/native_campaign_ai_perception_group_link_tu15.cpp
        src/campaign/ai/native_campaign_ai_perception_classify_tu15.cpp
        src/campaign/ai/native_campaign_ai_perception_group_pressure_tu15.cpp
        src/campaign/ai/native_campaign_ai_perception_group_actor_tu15.cpp
        src/campaign/ai/native_campaign_ai_perception_group_initialize_tu15.cpp
        src/campaign/ai/native_campaign_ai_perception_group_storage_tu15.cpp
        src/campaign/ai/native_campaign_object_world_point_tu15.cpp
        src/campaign/ai/native_campaign_ai_clump_retirement_tu15.cpp
        src/campaign/ai/native_campaign_ai_clump_callbacks_tu15.cpp)
    target_include_directories(native_campaign_ai_perception_lookup_tu15_tests PRIVATE include)
    target_compile_features(native_campaign_ai_perception_lookup_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_ai_perception_lookup_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_campaign_ai_perception_lookup_tu15_tests PRIVATE
        nlohmann_json::nlohmann_json halo2_native_tu15_data_array_delete)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_perception_lookup_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_perception_lookup_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_campaign_ai_perception_lookup_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_perception_lookup_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_perception_lookup_tu15.json.gz")
endif()
