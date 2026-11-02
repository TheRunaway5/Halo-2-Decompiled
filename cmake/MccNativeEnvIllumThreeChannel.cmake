set(HALO2_MCC_NATIVE_ENV_ILLUM_AUTHORITY "" CACHE FILEPATH
    "Optional complete paired environment/illumination census for live fixture export")
add_executable(mcc_native_env_illum_three_channel_probe
    tests/mcc_native_env_illum_three_channel_tests.cpp
    src/rendering/mcc_native_env_illum_three_channel.cpp src/assets/cache.cpp)
target_include_directories(mcc_native_env_illum_three_channel_probe PRIVATE include src)
target_compile_definitions(mcc_native_env_illum_three_channel_probe PRIVATE HALO2_CACHE_BYTES_ONLY=1)
target_link_libraries(mcc_native_env_illum_three_channel_probe PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
add_test(NAME mcc_native_env_illum_three_channel COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_native_env_illum_three_channel_tests.py
    --repo-root "${CMAKE_CURRENT_SOURCE_DIR}"
    --test-executable $<TARGET_FILE:mcc_native_env_illum_three_channel_probe>
    --native-root "${HALO2_MCC_NATIVE_LIBRARY_DIRECTORY}"
    --authority "${HALO2_MCC_NATIVE_ENV_ILLUM_AUTHORITY}"
    --report-output "${CMAKE_CURRENT_BINARY_DIR}/env-illum.json")
set_tests_properties(mcc_native_env_illum_three_channel PROPERTIES TIMEOUT 180)
add_test(NAME mcc_native_env_illum_three_channel_generator COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_native_env_illum_three_channel.py --check)
