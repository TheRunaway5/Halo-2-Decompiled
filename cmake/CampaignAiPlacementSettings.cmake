target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_campaign_ai_placement_settings_tu15_generated.cpp)
if(BUILD_TESTING)
    add_executable(campaign_ai_placement_settings_tu15_tests
        tests/campaign_ai_placement_settings_tu15_tests.cpp
        src/campaign/ai/source_campaign_ai_placement_settings_tu15_generated.cpp
        src/runtime/source_runtime_memory.cpp)
    target_include_directories(campaign_ai_placement_settings_tu15_tests PRIVATE include src)
    target_link_libraries(campaign_ai_placement_settings_tu15_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    if(MSVC)
        target_compile_options(campaign_ai_placement_settings_tu15_tests PRIVATE /fp:strict)
    else()
        target_compile_options(campaign_ai_placement_settings_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME original_campaign_ai_placement_settings_tu15
        COMMAND campaign_ai_placement_settings_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_placement_settings_tu15.json")
endif()
