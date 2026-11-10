if(BUILD_TESTING)
    add_custom_target(native_geometry_visibility_tests)
    add_test(NAME native_geometry_visibility_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_geometry_visibility_replacements.py")
    set_tests_properties(native_geometry_visibility_binding PROPERTIES TIMEOUT 120)
    set(_halo2_geometry_visibility_sanitizers)
    set(_halo2_geometry_visibility_values)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_geometry_visibility_sanitizers --sanitizers --semantic-mutants)
        list(APPEND _halo2_geometry_visibility_values --sanitizers)
    endif()
    add_test(NAME original_tu15_native_geometry_visibility
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_geometry_visibility_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-geometry-visibility.json"
        --prefixes --require-binding ${_halo2_geometry_visibility_sanitizers})
    set_tests_properties(original_tu15_native_geometry_visibility PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_geometry_visibility_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_geometry_visibility_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-geometry-visibility-values.json"
        --require-binding ${_halo2_geometry_visibility_values})
    set_tests_properties(original_tu15_native_geometry_visibility_values PROPERTIES TIMEOUT 180)
    add_test(NAME original_tu15_native_geometry_visibility_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_geometry_visibility_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-geometry-visibility-generation.json"
        --require-binding)
    set_tests_properties(original_tu15_native_geometry_visibility_generation PROPERTIES TIMEOUT 300)
    add_test(NAME original_tu15_native_geometry_visibility_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_geometry_visibility_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-geometry-visibility-release.json"
        --prefixes --require-binding)
    set_tests_properties(original_tu15_native_geometry_visibility_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
endif()
