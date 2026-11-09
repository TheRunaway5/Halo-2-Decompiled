if(BUILD_TESTING)
    add_custom_target(native_object_node_hierarchy_tests)
    set(_halo2_object_node_hierarchy_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_object_node_hierarchy_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_object_node_hierarchy
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_node_hierarchy_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-node-hierarchy.json"
        ${_halo2_object_node_hierarchy_sanitizers})
    set_tests_properties(original_tu15_native_object_node_hierarchy PROPERTIES TIMEOUT 240)
    unset(_halo2_object_node_hierarchy_sanitizers)
endif()
