target_sources(halo2_systems PRIVATE
    src/campaign/ai/native_campaign_unit_animation_reset_tu15.cpp
    src/campaign/ai/native_campaign_unit_animation_reset_tu15_binding.cpp)
if(BUILD_TESTING)
    add_executable(native_campaign_unit_animation_reset_tu15_tests
        tests/native_campaign_unit_animation_reset_tu15_tests.cpp
        src/campaign/ai/native_campaign_unit_animation_reset_tu15.cpp
        src/campaign/ai/native_campaign_unit_animation_reset_tu15_binding.cpp)
    target_include_directories(native_campaign_unit_animation_reset_tu15_tests PRIVATE include)
    target_link_libraries(native_campaign_unit_animation_reset_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_native_campaign_unit_animation_reset_tu15
        COMMAND native_campaign_unit_animation_reset_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_unit_animation_reset_tu15.json")
endif()
