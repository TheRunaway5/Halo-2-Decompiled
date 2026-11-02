if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    if(NOT HALO2_SHADOW_BASIS_ORACLE_SCRIPT)
        set(HALO2_SHADOW_BASIS_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_shadow_basis.py")
    endif()
    if(NOT HALO2_SHADOW_BASIS_FIXTURE_PATH)
        set(HALO2_SHADOW_BASIS_FIXTURE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_shadow_basis.json.gz")
    endif()
    add_executable(native_tu15_shadow_basis_tests tests/native_tu15_shadow_basis_tests.cpp
        src/rendering/shadows/native_tu15_shadow_basis_32340.cpp src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
    target_include_directories(native_tu15_shadow_basis_tests PRIVATE include)
    target_compile_features(native_tu15_shadow_basis_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_shadow_basis_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_tu15_shadow_basis_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_shadow_basis_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_shadow_basis_tests PRIVATE -fsanitize=address,undefined)
    endif()
    set(_halo2_shadow_fixture_output)
    if(HALO2_SHADOW_BASIS_FIXTURE_OUTPUT)
        list(APPEND _halo2_shadow_fixture_output --fixture-output "${HALO2_SHADOW_BASIS_FIXTURE_OUTPUT}")
    endif()
    add_test(NAME original_tu15_native_shadow_basis COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${HALO2_SHADOW_BASIS_ORACLE_SCRIPT}"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_shadow_basis_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-shadow-basis.json" ${_halo2_shadow_fixture_output})
    set_tests_properties(original_tu15_native_shadow_basis PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME native_tu15_shadow_basis_fixture COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_tu15_shadow_basis_fixture.py"
        --fixture "${HALO2_SHADOW_BASIS_FIXTURE_PATH}" --native $<TARGET_FILE:native_tu15_shadow_basis_tests>)
    set_tests_properties(native_tu15_shadow_basis_fixture PROPERTIES TIMEOUT 60)
endif()
