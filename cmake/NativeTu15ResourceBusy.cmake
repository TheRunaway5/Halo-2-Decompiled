if(BUILD_TESTING)
    add_custom_target(native_resource_busy_tests)
    set(_halo2_resource_busy_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_resource_busy_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_resource_busy
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_resource_busy_tu15.py"
        --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-resource-busy.json"
        ${_halo2_resource_busy_sanitizers})
    set_tests_properties(original_tu15_native_resource_busy PROPERTIES TIMEOUT 420)
    unset(_halo2_resource_busy_sanitizers)
endif()
