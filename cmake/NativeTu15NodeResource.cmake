if(BUILD_TESTING)
    add_custom_target(native_node_resource_tests)
    set(_halo2_node_resource_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_node_resource_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_node_resource
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_node_resource.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-node-resource.json"
        --require-binding ${_halo2_node_resource_sanitizers})
    set_tests_properties(original_tu15_native_node_resource PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    unset(_halo2_node_resource_sanitizers)
endif()
