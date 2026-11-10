if(BUILD_TESTING)
    add_custom_target(native_camera_instance_query_tests)
    set(_halo2_camera_query_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_camera_query_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_camera_instance_query
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_instance_query_complete_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-instance-query"
        --require-binding ${_halo2_camera_query_sanitizers})
    set_tests_properties(original_tu15_native_camera_instance_query PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_camera_instance_query_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_instance_query_public_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-instance-query-values")
    set_tests_properties(original_tu15_native_camera_instance_query_values PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_camera_instance_query_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_instance_query_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-instance-query-generation.json")
    set_tests_properties(original_tu15_native_camera_instance_query_generation PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_camera_instance_query_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_instance_query_complete_tu15.py"
            --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-instance-query-release"
            --require-binding)
        set_tests_properties(original_tu15_native_camera_instance_query_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_camera_query_sanitizers)
endif()
