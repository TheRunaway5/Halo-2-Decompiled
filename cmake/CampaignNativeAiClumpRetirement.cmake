include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15DataArrayDelete.cmake")
target_link_libraries(halo2_systems PRIVATE halo2_native_tu15_data_array_delete)
target_sources(halo2_systems PRIVATE
    src/campaign/ai/native_campaign_ai_clump_retirement_tu15.cpp
    src/campaign/ai/native_campaign_ai_clump_callbacks_tu15.cpp
    src/campaign/ai/native_campaign_ai_clump_retirement_tu15_binding.cpp)
if(BUILD_TESTING)
    add_executable(native_campaign_ai_clump_retirement_tu15_tests
        tests/native_campaign_ai_clump_retirement_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_clump_retirement_tu15.cpp
        src/campaign/ai/native_campaign_ai_clump_callbacks_tu15.cpp
        src/campaign/ai/native_campaign_ai_clump_retirement_tu15_binding.cpp)
    target_include_directories(native_campaign_ai_clump_retirement_tu15_tests PRIVATE include)
    target_link_libraries(native_campaign_ai_clump_retirement_tu15_tests PRIVATE
        nlohmann_json::nlohmann_json halo2_native_tu15_data_array_delete)
    add_test(NAME original_native_campaign_ai_clump_retirement_tu15
        COMMAND native_campaign_ai_clump_retirement_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_clump_retirement_tu15.json")
    add_test(NAME original_native_campaign_ai_clump_retirement_tu15_callbacks
        COMMAND native_campaign_ai_clump_retirement_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_clump_retirement_callbacks_tu15.json")
    add_executable(native_campaign_ai_clump_detach_actor_tu15_tests
        tests/native_campaign_ai_clump_detach_actor_tu15_tests.cpp
        src/campaign/ai/native_campaign_ai_clump_retirement_tu15.cpp
        src/campaign/ai/native_campaign_ai_clump_callbacks_tu15.cpp
        src/campaign/ai/native_campaign_ai_clump_retirement_tu15_binding.cpp)
    target_include_directories(native_campaign_ai_clump_detach_actor_tu15_tests PRIVATE include)
    target_compile_features(native_campaign_ai_clump_detach_actor_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(native_campaign_ai_clump_detach_actor_tu15_tests PRIVATE
        nlohmann_json::nlohmann_json halo2_native_tu15_data_array_delete)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_clump_detach_actor_tu15_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_clump_detach_actor_tu15_tests PRIVATE
            -fsanitize=address,undefined)
    endif()
    add_test(NAME native_campaign_ai_clump_detach_actor_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_clump_detach_actor_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_clump_detach_actor_tu15.json.gz")
    set_tests_properties(native_campaign_ai_clump_detach_actor_tu15 PROPERTIES TIMEOUT 300)
endif()
