include_guard(GLOBAL)
target_sources(halo2_physics_world_runtime PRIVATE src/platform/native_tu15_crt_random.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_crt_random_tests
        tests/native_tu15_crt_random_tests.cpp src/platform/native_tu15_crt_random.cpp)
    target_compile_features(native_tu15_crt_random_tests PRIVATE cxx_std_20)
    target_include_directories(native_tu15_crt_random_tests PRIVATE include)
    target_link_libraries(native_tu15_crt_random_tests PRIVATE nlohmann_json::nlohmann_json)
    if(NOT MSVC AND HALO2_SANITIZERS)
        target_compile_options(native_tu15_crt_random_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_crt_random_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_crt_random
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_tu15_crt_random_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_crt_random.json.gz")
endif()
