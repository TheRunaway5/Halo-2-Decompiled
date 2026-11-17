if(BUILD_TESTING)
    add_custom_target(native_solver_hull_minimum_value_tests)
    set(_halo2_hull_minimum_value_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_hull_minimum_value_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_solver_hull_minimum_value
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_minimum_value_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --mutations --require-binding
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-minimum-value.json"
        ${_halo2_hull_minimum_value_sanitizers})
    set_tests_properties(original_tu15_native_solver_hull_minimum_value PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    unset(_halo2_hull_minimum_value_sanitizers)
endif()
