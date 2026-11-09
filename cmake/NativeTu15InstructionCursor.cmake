if(BUILD_TESTING)
    add_custom_target(native_instruction_cursor_tests)
    set(_halo2_cursor_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_cursor_sanitizers --sanitizers)
    endif()
    add_test(NAME native_tu15_instruction_cursor
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_instruction_cursor_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-instruction-cursor"
        ${_halo2_cursor_sanitizers})
    set_tests_properties(native_tu15_instruction_cursor PROPERTIES TIMEOUT 120)
    unset(_halo2_cursor_sanitizers)
endif()
