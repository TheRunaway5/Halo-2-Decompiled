if(BUILD_TESTING)
    add_custom_target(native_node_resource_lifecycle_tests)
    set(_halo2_node_lifecycle_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_node_lifecycle_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_node_resource_lifecycle
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_node_resource_lifecycle_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-node-resource-lifecycle.json"
        --require-binding ${_halo2_node_lifecycle_sanitizers})
    set_tests_properties(original_tu15_native_node_resource_lifecycle PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_node_resource_lifecycle_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_node_resource_lifecycle_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-node-resource-lifecycle-release.json"
            --require-binding)
        set_tests_properties(original_tu15_native_node_resource_lifecycle_release PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_node_lifecycle_sanitizers)
endif()
