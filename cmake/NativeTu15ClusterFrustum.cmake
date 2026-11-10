if(BUILD_TESTING)
    add_custom_target(native_cluster_frustum_tests)
    add_test(NAME native_cluster_frustum_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_cluster_frustum_replacements.py")
    set_tests_properties(native_cluster_frustum_binding PROPERTIES TIMEOUT 120)
    set(_halo2_cluster_frustum_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_cluster_frustum_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_cluster_frustum
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_cluster_frustum_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-cluster-frustum.json"
        --prefixes --require-binding ${_halo2_cluster_frustum_sanitizers})
    set_tests_properties(original_tu15_native_cluster_frustum PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    set(_halo2_cluster_values_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_cluster_values_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_cluster_frustum_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_cluster_frustum_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-cluster-frustum-values.json"
        --require-binding ${_halo2_cluster_values_sanitizers})
    set_tests_properties(original_tu15_native_cluster_frustum_values PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_cluster_frustum_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_cluster_frustum_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-cluster-frustum-generation.json")
    set_tests_properties(original_tu15_native_cluster_frustum_generation PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_cluster_frustum_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_cluster_frustum_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-cluster-frustum-release.json"
            --prefixes --require-binding)
        set_tests_properties(original_tu15_native_cluster_frustum_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_cluster_frustum_sanitizers)
    unset(_halo2_cluster_values_sanitizers)
endif()
