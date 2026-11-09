if(BUILD_TESTING)
    add_custom_target(native_millisecond_clock_tests)
    set(_halo2_millisecond_clock_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_millisecond_clock_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_millisecond_clock
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_millisecond_clock.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-millisecond-clock.json"
        --require-binding ${_halo2_millisecond_clock_sanitizers})
    set_tests_properties(original_tu15_native_millisecond_clock PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    unset(_halo2_millisecond_clock_sanitizers)
endif()
