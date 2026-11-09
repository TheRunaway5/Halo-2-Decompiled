if(BUILD_TESTING)
    add_custom_target(native_tu15_tick_conversion_tests)
    set(_halo2_tick_conversion_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_tick_conversion_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_tick_conversion
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tick_conversion_tu15.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --require-binding
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-tick-conversion.json
        ${_halo2_tick_conversion_sanitizers})
    set_tests_properties(original_tu15_native_tick_conversion PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        # The native MUL escape flags must also be checked without sanitizer
        # instrumentation changing the host flags left by memory adapters.
        add_test(NAME original_tu15_native_tick_conversion_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tick_conversion_tu15.py
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --require-binding
            --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-tick-conversion-release.json)
        set_tests_properties(original_tu15_native_tick_conversion_release PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_tick_conversion_sanitizers)
endif()
