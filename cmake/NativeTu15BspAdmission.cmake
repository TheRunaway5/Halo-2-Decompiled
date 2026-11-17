if(BUILD_TESTING)
    add_custom_target(native_bsp_admission_tests)
    add_test(NAME native_bsp_admission_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_bsp_admission_replacements.py")
    set_tests_properties(native_bsp_admission_binding PROPERTIES TIMEOUT 120)
    set(_halo2_bsp_admission_sanitizers)
    set(_halo2_bsp_admission_values)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_bsp_admission_sanitizers --sanitizers --semantic-mutants)
        list(APPEND _halo2_bsp_admission_values --sanitizers)
    endif()
    add_test(NAME original_tu15_native_bsp_admission
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_admission_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-admission.json"
        --prefixes --require-binding ${_halo2_bsp_admission_sanitizers})
    set_tests_properties(original_tu15_native_bsp_admission PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_bsp_admission_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_admission_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-admission-values.json"
        --require-binding ${_halo2_bsp_admission_values})
    set_tests_properties(original_tu15_native_bsp_admission_values PROPERTIES TIMEOUT 180)
    add_test(NAME original_tu15_native_bsp_admission_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_admission_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-admission-generation.json"
        --require-binding)
    set_tests_properties(original_tu15_native_bsp_admission_generation PROPERTIES TIMEOUT 300)
    add_test(NAME original_tu15_native_bsp_admission_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_admission_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-admission-release.json"
        --prefixes --require-binding)
    set_tests_properties(original_tu15_native_bsp_admission_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
endif()
