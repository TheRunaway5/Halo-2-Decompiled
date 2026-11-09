if(BUILD_TESTING)
    add_custom_target(native_kernel_yield_tests)
    set(_halo2_kernel_yield_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_kernel_yield_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_kernel_yield
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_kernel_yield.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-kernel-yield.json"
        --require-binding ${_halo2_kernel_yield_sanitizers})
    set_tests_properties(original_tu15_native_kernel_yield PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_kernel_yield_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_kernel_yield.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-kernel-yield-release.json"
            --require-binding)
        set_tests_properties(original_tu15_native_kernel_yield_release PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_kernel_yield_sanitizers)
endif()
