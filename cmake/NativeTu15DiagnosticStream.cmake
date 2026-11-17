if(BUILD_TESTING)
    add_custom_target(native_diagnostic_stream_tests)
    add_test(NAME native_diagnostic_stream_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_diagnostic_stream_replacements.py")
    set_tests_properties(native_diagnostic_stream_binding PROPERTIES TIMEOUT 120)
    set(_halo2_diagnostic_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_diagnostic_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_diagnostic_stream
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_diagnostic_stream_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-diagnostic-stream.json"
        --buffered-writer --prefixes --require-binding ${_halo2_diagnostic_sanitizers})
    set_tests_properties(original_tu15_native_diagnostic_stream PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_diagnostic_stream_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_diagnostic_stream_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-diagnostic-stream-release.json"
        --buffered-writer --prefixes --require-binding)
    set_tests_properties(original_tu15_native_diagnostic_stream_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_diagnostic_stream_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_diagnostic_stream_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-diagnostic-stream-generation.json"
        --require-binding)
    set_tests_properties(original_tu15_native_diagnostic_stream_generation PROPERTIES TIMEOUT 300)
endif()
