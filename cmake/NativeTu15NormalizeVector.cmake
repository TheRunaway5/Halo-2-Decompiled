# Qualify the full state boundary before binding generated renderer callers.
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_normalize_vector_tests tests/native_tu15_normalize_vector_tests.cpp
        src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
    target_include_directories(native_tu15_normalize_vector_tests PRIVATE include src)
    target_compile_features(native_tu15_normalize_vector_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_normalize_vector_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_tu15_normalize_vector_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_normalize_vector_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_normalize_vector_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_mp11_native_normalize_vector COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_normalize_vector.py"
        --tu15 "${HALO2_TU15_GAMEPLAY_XBE}"
        --mp11 "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"
        --native $<TARGET_FILE:native_tu15_normalize_vector_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-normalize-vector.json")
    set_tests_properties(original_tu15_mp11_native_normalize_vector PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(native_tu15_normalize_storage_tests tests/native_tu15_normalize_storage_tests.cpp
        src/runtime/source_runtime_memory.cpp src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
    target_include_directories(native_tu15_normalize_storage_tests PRIVATE include src)
    target_compile_features(native_tu15_normalize_storage_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_normalize_storage_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_tu15_normalize_storage_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    add_test(NAME original_tu15_native_normalize_real_storage COMMAND native_tu15_normalize_storage_tests)
    set_tests_properties(original_tu15_native_normalize_real_storage PROPERTIES TIMEOUT 20)
endif()
