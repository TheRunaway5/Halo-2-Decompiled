if(BUILD_TESTING)
    add_custom_target(native_mopp_ray_data_tests)
    add_test(NAME original_tu15_native_mopp_ray_data
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_traversal_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-data")
    set_tests_properties(original_tu15_native_mopp_ray_data PROPERTIES
        TIMEOUT 900 SKIP_RETURN_CODE 77)
endif()
