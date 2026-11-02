set(HALO2_MCC_NATIVE_ALPHA_ENVIRONMENT_AUTHORITY "" CACHE FILEPATH
    "Optional complete paired alpha/environment census for live fixture export")
add_executable(mcc_native_alpha_environment_probe
    tests/mcc_native_alpha_environment_tests.cpp
    src/rendering/mcc_native_alpha_environment.cpp src/assets/cache.cpp)
target_include_directories(mcc_native_alpha_environment_probe PRIVATE include src)
target_compile_definitions(mcc_native_alpha_environment_probe PRIVATE HALO2_CACHE_BYTES_ONLY=1)
target_link_libraries(mcc_native_alpha_environment_probe PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
add_test(NAME mcc_native_alpha_environment COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_native_alpha_environment_tests.py
    --repo-root "${CMAKE_CURRENT_SOURCE_DIR}"
    --test-executable $<TARGET_FILE:mcc_native_alpha_environment_probe>
    --native-root "${HALO2_MCC_NATIVE_LIBRARY_DIRECTORY}"
    --authority "${HALO2_MCC_NATIVE_ALPHA_ENVIRONMENT_AUTHORITY}"
    --report-output "${CMAKE_CURRENT_BINARY_DIR}/alpha-environment.json")
set_tests_properties(mcc_native_alpha_environment PROPERTIES TIMEOUT 180)
add_test(NAME mcc_native_alpha_environment_generator COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_native_alpha_environment.py --check)
