if(BUILD_TESTING)
    add_custom_target(native_simplex_paired_tests)
    set(_halo2_simplex_paired_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_simplex_paired_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_simplex_paired
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_simplex_paired_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-simplex-paired.json"
        ${_halo2_simplex_paired_sanitizers})
    set_tests_properties(original_tu15_native_simplex_paired PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    unset(_halo2_simplex_paired_sanitizers)
endif()
