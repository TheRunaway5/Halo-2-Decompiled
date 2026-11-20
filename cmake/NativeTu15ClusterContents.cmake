if(BUILD_TESTING)
    set(_halo2_cluster_contents_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_cluster_contents_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_cluster_contents
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_cluster_contents_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-cluster-contents.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_cluster_contents PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_prt_partition
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_prt_partition_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-prt-partition.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_prt_partition PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_object_fade
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_fade_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-fade.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_object_fade PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_object_draw_flags
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_draw_flags_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-draw-flags.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_object_draw_flags PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_object_pass
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_pass_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-pass.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_object_pass PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_model_hierarchy
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_model_hierarchy_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-hierarchy.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_model_hierarchy PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_object_submit
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_submit_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-submit.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_object_submit PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_light_shape
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_light_shape_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-light-shape.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_light_shape PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_view_reset
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_view_reset_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-view-reset.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_view_reset PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_object_hierarchy_root
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_hierarchy_root_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-hierarchy-root.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_object_hierarchy_root PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_view_setup
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_view_setup_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-view-setup.json"
        --prefixes --require-binding ${_halo2_cluster_contents_sanitizers})
    set_tests_properties(original_tu15_native_view_setup PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
endif()
