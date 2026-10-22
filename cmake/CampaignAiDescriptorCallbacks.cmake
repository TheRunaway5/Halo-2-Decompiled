if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(campaign_ai_descriptor_callbacks_tu15_tests
        tests/campaign_ai_descriptor_callbacks_tu15_tests.cpp src/campaign/ai/source_ai_frame_tu15_generated.cpp
        src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp
        src/platform/native_tu15_final_startup_storage_data.cpp src/runtime/source_runtime_memory.cpp
        src/campaign/scripting/native_mcc_script_builtin.cpp)
    target_include_directories(campaign_ai_descriptor_callbacks_tu15_tests PRIVATE include src)
    target_link_libraries(campaign_ai_descriptor_callbacks_tu15_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    target_compile_options(campaign_ai_descriptor_callbacks_tu15_tests PRIVATE
        -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math -O0)
    target_link_options(campaign_ai_descriptor_callbacks_tu15_tests PRIVATE -Wl,--gc-sections)
    add_test(NAME original_campaign_ai_descriptor_callbacks_tu15 COMMAND campaign_ai_descriptor_callbacks_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_descriptor_callbacks_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_object_tracking_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_tracking_dispatch_tu15.json")
endif()
