target_sources(halo2_physics_world_runtime PRIVATE
    src/campaign/native_campaign_screen_lifecycle_tu15.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_library(native_campaign_screen_lifecycle_callers OBJECT
        src/campaign/source_campaign_screen_runtime_tu15.cpp)
    set_target_properties(native_campaign_screen_lifecycle_callers PROPERTIES EXCLUDE_FROM_ALL TRUE)
    target_include_directories(native_campaign_screen_lifecycle_callers PRIVATE include src)
    add_executable(native_campaign_screen_lifecycle_tu15_tests
        tests/native_campaign_screen_lifecycle_tu15_tests.cpp
        src/campaign/native_campaign_screen_lifecycle_tu15.cpp)
    target_include_directories(native_campaign_screen_lifecycle_tu15_tests PRIVATE include)
    target_link_libraries(native_campaign_screen_lifecycle_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_dependencies(native_campaign_screen_lifecycle_tu15_tests native_campaign_screen_lifecycle_callers)
    add_test(NAME native_campaign_screen_lifecycle_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_screen_lifecycle_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_screen_lifecycle_tu15.json.gz")
    set_tests_properties(native_campaign_screen_lifecycle_tu15 PROPERTIES TIMEOUT 90)
endif()
