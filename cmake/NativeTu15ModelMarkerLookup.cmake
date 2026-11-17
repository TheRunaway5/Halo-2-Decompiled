if(BUILD_TESTING)
    add_custom_target(native_model_marker_lookup_tests)
    set(_halo2_model_marker_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_model_marker_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_model_marker_lookup
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_model_marker_lookup_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-marker-lookup"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies
        --prefixes --mmx ${_halo2_model_marker_sanitizers})
    set_tests_properties(original_tu15_native_model_marker_lookup PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_model_marker_lookup_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_model_marker_lookup_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-marker-lookup-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
    set_tests_properties(original_tu15_native_model_marker_lookup_generation PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_model_marker_lookup_sandbox
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sandbox_dispatch_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-marker-lookup-sandbox"
        --compiler "${CMAKE_CXX_COMPILER}")
    set_tests_properties(original_tu15_native_model_marker_lookup_sandbox PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_model_marker_lookup_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_model_marker_lookup_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-marker-lookup-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies --prefixes --mmx)
        set_tests_properties(original_tu15_native_model_marker_lookup_release PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_model_marker_sanitizers)
endif()
