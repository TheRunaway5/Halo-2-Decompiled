if(BUILD_TESTING)
    add_custom_target(native_stack_probe_tests)
    set(_halo2_stack_probe_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_stack_probe_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_stack_probe
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_stack_probe_abi_tu15.py"
        --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-stack-probe-abi.json"
        ${_halo2_stack_probe_sanitizers})
    set_tests_properties(original_tu15_native_stack_probe PROPERTIES TIMEOUT 420)
    unset(_halo2_stack_probe_sanitizers)
endif()
