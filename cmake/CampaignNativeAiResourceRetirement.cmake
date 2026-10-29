include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15DataArrayDelete.cmake")
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/native_campaign_ai_resource_retirement_tu15.cpp)
target_link_libraries(halo2_physics_world_runtime PRIVATE halo2_native_tu15_data_array_delete)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_campaign_ai_resource_retirement_tu15_tests
        tests/native_campaign_ai_resource_retirement_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_resource_retirement_tu15.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_physical_memory.cpp)
    target_include_directories(native_campaign_ai_resource_retirement_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_ai_resource_retirement_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(native_campaign_ai_resource_retirement_tu15_tests PRIVATE
        halo2_native_tu15_data_array_delete halo2_snapshot_storage nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_resource_retirement_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_resource_retirement_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_campaign_ai_resource_retirement_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_resource_retirement_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_resource_retirement_tu15.json.gz")
endif()
