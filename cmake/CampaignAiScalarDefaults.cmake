if(TARGET halo2_physics_world_runtime)
    target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_campaign_ai_scalar_defaults_tu15.cpp)
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(campaign_ai_scalar_defaults_tu15_tests
        tests/campaign_ai_scalar_defaults_tu15_tests.cpp src/campaign/ai/source_campaign_ai_scalar_defaults_tu15.cpp
        src/platform/native_tu15_final_startup_storage_data.cpp src/runtime/source_runtime_memory.cpp
        src/campaign/scripting/native_mcc_script_builtin.cpp)
    target_include_directories(campaign_ai_scalar_defaults_tu15_tests PRIVATE include src)
    target_link_libraries(campaign_ai_scalar_defaults_tu15_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    target_compile_options(campaign_ai_scalar_defaults_tu15_tests PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(campaign_ai_scalar_defaults_tu15_tests PRIVATE -Wl,--gc-sections)
    add_test(NAME original_campaign_ai_scalar_defaults_tu15 COMMAND campaign_ai_scalar_defaults_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_scalar_defaults_tu15.json")
endif()
