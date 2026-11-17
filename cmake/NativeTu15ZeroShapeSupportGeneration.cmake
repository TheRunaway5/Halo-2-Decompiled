if(BUILD_TESTING)
    add_custom_target(native_zero_shape_support_generation_tests)
    add_test(NAME original_tu15_native_zero_shape_support_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_zero_shape_support_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-zero-shape-support-generation.json")
    set_tests_properties(original_tu15_native_zero_shape_support_generation PROPERTIES TIMEOUT 300)
endif()
