if(BUILD_TESTING)
    add_custom_target(native_diagnostic_logger_owner_tests)
    add_test(NAME native_diagnostic_logger_world_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_diagnostic_logger_world_replacements.py")
    set_tests_properties(native_diagnostic_logger_world_binding PROPERTIES TIMEOUT 120)
    set(_halo2_diagnostic_logger_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_diagnostic_logger_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_diagnostic_logger_owner
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_diagnostic_logger_owner_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-diagnostic-logger-owner.json"
        ${_halo2_diagnostic_logger_sanitizers})
    add_test(NAME original_tu15_native_diagnostic_logger_owner_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_diagnostic_logger_owner_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-diagnostic-logger-owner-release.json")
    add_test(NAME original_tu15_native_diagnostic_logger_world
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_diagnostic_logger_world_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-diagnostic-logger-world.json"
        --require-binding ${_halo2_diagnostic_logger_sanitizers})
    set_tests_properties(original_tu15_native_diagnostic_logger_owner
        original_tu15_native_diagnostic_logger_owner_release
        original_tu15_native_diagnostic_logger_world PROPERTIES TIMEOUT 600)
endif()
