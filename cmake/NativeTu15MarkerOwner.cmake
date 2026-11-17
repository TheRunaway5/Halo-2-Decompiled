if(BUILD_TESTING)
    add_custom_target(native_marker_owner_tests)
    set(_halo2_marker_owner_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_marker_owner_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_marker_owner
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_marker_owner_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-marker-owner"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies
        --prefixes --public-value ${_halo2_marker_owner_sanitizers})
    set_tests_properties(original_tu15_native_marker_owner PROPERTIES TIMEOUT 2400)
    add_test(NAME original_tu15_native_marker_owner_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_marker_owner_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-marker-owner-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
    set_tests_properties(original_tu15_native_marker_owner_generation PROPERTIES TIMEOUT 1800)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_marker_owner_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_marker_owner_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-marker-owner-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies --prefixes --public-value)
        set_tests_properties(original_tu15_native_marker_owner_release PROPERTIES TIMEOUT 2400)
    endif()
    unset(_halo2_marker_owner_sanitizers)
endif()
