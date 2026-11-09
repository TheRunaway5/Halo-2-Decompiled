if(BUILD_TESTING)
    add_custom_target(native_node_descriptor_lookup_tests)
    set(_halo2_node_descriptor_lookup_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_node_descriptor_lookup_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_node_descriptor_lookup
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_node_descriptor_lookup_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-node-descriptor-lookup.json"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_node_descriptor_lookup_sanitizers})
    set_tests_properties(original_tu15_native_node_descriptor_lookup PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_node_descriptor_lookup_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_node_descriptor_lookup_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-node-descriptor-lookup-release.json"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
        set_tests_properties(original_tu15_native_node_descriptor_lookup_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_node_descriptor_lookup_sanitizers)
endif()
