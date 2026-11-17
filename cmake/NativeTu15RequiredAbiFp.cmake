if(BUILD_TESTING)
    add_custom_target(native_required_abi_fp_tests)
    set(_halo2_required_abi_fp_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_required_abi_fp_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_required_abi_fp
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_required_abi_fp_changing_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-required-abi-fp"
        --compiler "${CMAKE_CXX_COMPILER}" ${_halo2_required_abi_fp_sanitizers})
    set_tests_properties(original_tu15_native_required_abi_fp PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_required_abi_fp_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_required_abi_fp_changing_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-required-abi-fp-release"
            --compiler "${CMAKE_CXX_COMPILER}")
        set_tests_properties(original_tu15_native_required_abi_fp_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_required_abi_fp_sanitizers)
endif()
