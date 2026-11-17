if(BUILD_TESTING)
    add_custom_target(native_solver_hull_support_pair_generation_tests)
    add_test(NAME original_tu15_native_solver_hull_support_pair_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_support_pair_generation_tu15.py"
        --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-support-pair-generation.json")
    set_tests_properties(original_tu15_native_solver_hull_support_pair_generation PROPERTIES TIMEOUT 360)
endif()
