if(BUILD_TESTING)
    add_custom_target(native_simplex_boundary_generation_tests)
    add_test(NAME original_tu15_native_simplex_boundary_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_simplex_boundary_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-simplex-boundary-generation.json")
    set_tests_properties(original_tu15_native_simplex_boundary_generation PROPERTIES TIMEOUT 300)
endif()
