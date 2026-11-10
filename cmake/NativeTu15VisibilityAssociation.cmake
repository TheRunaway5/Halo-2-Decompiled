if(BUILD_TESTING)
    add_custom_target(native_visibility_association_tests)
    add_test(NAME native_visibility_association_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_visibility_association_replacements.py")
    set_tests_properties(native_visibility_association_binding PROPERTIES TIMEOUT 120)
    set(_halo2_association_sanitizers)
    set(_halo2_association_values)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_association_sanitizers --sanitizers --semantic-mutants)
        list(APPEND _halo2_association_values --sanitizers)
    endif()
    add_test(NAME original_tu15_native_visibility_association
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_visibility_association_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-visibility-association.json"
        --prefixes --require-binding ${_halo2_association_sanitizers})
    set_tests_properties(original_tu15_native_visibility_association PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_visibility_association_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_visibility_association_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-visibility-association-values.json"
        --require-binding ${_halo2_association_values})
    set_tests_properties(original_tu15_native_visibility_association_values PROPERTIES TIMEOUT 180)
    add_test(NAME original_tu15_native_visibility_association_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_visibility_association_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-visibility-association-generation.json"
        --require-binding)
    set_tests_properties(original_tu15_native_visibility_association_generation PROPERTIES TIMEOUT 300)
    add_test(NAME original_tu15_native_visibility_association_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_visibility_association_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-visibility-association-release.json"
        --prefixes --require-binding)
    set_tests_properties(original_tu15_native_visibility_association_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
endif()
