if(BUILD_TESTING)
    add_custom_target(native_solver_hull_minimum_generation_tests)
    add_test(NAME original_tu15_native_solver_hull_minimum_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_minimum_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-minimum-generation.json")
    set_tests_properties(original_tu15_native_solver_hull_minimum_generation PROPERTIES TIMEOUT 1200)
endif()
