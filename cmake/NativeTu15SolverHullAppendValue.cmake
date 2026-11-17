if(BUILD_TESTING)
    add_custom_target(native_solver_hull_append_value_tests)
    add_test(NAME original_tu15_native_solver_hull_append_value
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_context_value_tu15.py"
        --entry append --mode declared
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-append-value.json")
    set_tests_properties(original_tu15_native_solver_hull_append_value PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
endif()
