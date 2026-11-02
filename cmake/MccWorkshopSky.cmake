add_executable(mcc_native_sky_cooked_probe tests/mcc_native_sky_cooked_tests.cpp
    src/rendering/mcc_native_world_animated.cpp src/assets/cache.cpp)
target_include_directories(mcc_native_sky_cooked_probe PRIVATE include src)
target_compile_definitions(mcc_native_sky_cooked_probe PRIVATE HALO2_CACHE_BYTES_ONLY=1)
target_link_libraries(mcc_native_sky_cooked_probe PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
add_test(NAME mcc_native_sky_cooked COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_native_sky_cooked_tests.py
    --repo-root "${CMAKE_CURRENT_SOURCE_DIR}" --test-executable $<TARGET_FILE:mcc_native_sky_cooked_probe>)
set_tests_properties(mcc_native_sky_cooked PROPERTIES TIMEOUT 180)
add_test(NAME mcc_native_sky_cooked_generator COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_native_sky_profiles.py --check)
add_executable(workshop_sky_curve_tests tests/workshop_sky_curve_tests.cpp)
target_link_libraries(workshop_sky_curve_tests PRIVATE halo2_cache)
add_test(NAME workshop_sky_curves COMMAND workshop_sky_curve_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_sky_curves_tu15.tsv
    ${HALO2_WORKSHOP_TEST_CACHE}/native-shaders/function-tables.bin)
