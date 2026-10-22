target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_campaign_ai_conditions_tu15_generated.cpp)
if(BUILD_TESTING)
    add_executable(campaign_ai_conditions_tu15_tests
        tests/campaign_ai_conditions_tu15_tests.cpp
        src/campaign/ai/source_campaign_ai_conditions_tu15_generated.cpp
        src/campaign/ai/source_ai_frame_tu15_generated.cpp src/campaign/source_campaign_runtime_tu15_generated.cpp
        src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp
        src/campaign/scripting/native_mcc_script_builtin.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(campaign_ai_conditions_tu15_tests PRIVATE include src)
    target_link_libraries(campaign_ai_conditions_tu15_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    if(MSVC)
        target_compile_options(campaign_ai_conditions_tu15_tests PRIVATE /fp:strict /Od)
    else()
        target_compile_options(campaign_ai_conditions_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math -O0)
    endif()
    add_test(NAME original_campaign_ai_conditions_tu15 COMMAND campaign_ai_conditions_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_conditions_tu15.json")
endif()
