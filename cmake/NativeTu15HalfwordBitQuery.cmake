if(BUILD_TESTING)
    add_custom_target(native_halfword_bit_query_tests)
    set(_halo2_halfword_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_halfword_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_halfword_bit_query
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_halfword_bit_query_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-halfword-bit-query.json"
        --require-binding ${_halo2_halfword_sanitizers})
    set_tests_properties(original_tu15_native_halfword_bit_query PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_halfword_bit_query_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_halfword_bit_query_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-halfword-bit-query-release.json"
            --require-binding)
        set_tests_properties(original_tu15_native_halfword_bit_query_release PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_halfword_sanitizers)
endif()
