if(BUILD_TESTING)
    add_custom_target(native_zero_shape_support_value_tests)
    set(_halo2_zero_shape_support_value_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_zero_shape_support_value_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_zero_shape_support_value
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_zero_shape_support_value_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-zero-shape-support-value.json"
        ${_halo2_zero_shape_support_value_sanitizers})
    set_tests_properties(original_tu15_native_zero_shape_support_value PROPERTIES TIMEOUT 180)
    unset(_halo2_zero_shape_support_value_sanitizers)
endif()
