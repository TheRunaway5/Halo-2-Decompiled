include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/CampaignNativeSessionTimers.cmake)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/native_campaign_session_timers_tu15_binding.cpp)
if(BUILD_TESTING)
    add_executable(native_campaign_session_timers_tu15_binding_tests
        tests/native_campaign_session_timers_tu15_binding_tests.cpp
        src/campaign/native_campaign_session_timers_tu15_binding.cpp
        src/campaign/native_campaign_session_timers_tu15.cpp)
    target_compile_features(native_campaign_session_timers_tu15_binding_tests PRIVATE cxx_std_20)
    target_include_directories(native_campaign_session_timers_tu15_binding_tests PRIVATE include)
    target_link_libraries(native_campaign_session_timers_tu15_binding_tests PRIVATE nlohmann_json::nlohmann_json)
    if(NOT MSVC AND HALO2_SANITIZERS)
        target_compile_options(native_campaign_session_timers_tu15_binding_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_session_timers_tu15_binding_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_campaign_session_timers_tu15_binding
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_session_timers_tu15_binding_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_session_timers_callers_tu15.json.gz")
endif()
