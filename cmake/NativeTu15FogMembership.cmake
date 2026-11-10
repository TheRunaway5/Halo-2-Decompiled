if(BUILD_TESTING)
    add_custom_target(native_fog_membership_tests)
    add_test(NAME native_fog_membership_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_fog_membership_replacements.py")
    set_tests_properties(native_fog_membership_binding PROPERTIES TIMEOUT 120)
    set(_halo2_fog_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_fog_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_fog_membership
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_fog_membership_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-fog-membership.json"
        --prefixes --require-binding ${_halo2_fog_sanitizers})
    set_tests_properties(original_tu15_native_fog_membership PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    set(_halo2_fog_value_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_fog_value_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_fog_membership_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_fog_membership_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-fog-membership-values.json"
        --require-binding ${_halo2_fog_value_sanitizers})
    set_tests_properties(original_tu15_native_fog_membership_values PROPERTIES TIMEOUT 180)
    add_test(NAME original_tu15_native_fog_membership_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_fog_membership_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-fog-membership-generation.json"
        --require-binding)
    set_tests_properties(original_tu15_native_fog_membership_generation PROPERTIES TIMEOUT 300)
    add_test(NAME original_tu15_native_fog_membership_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_fog_membership_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-fog-membership-release.json"
        --prefixes --require-binding)
    set_tests_properties(original_tu15_native_fog_membership_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
endif()
