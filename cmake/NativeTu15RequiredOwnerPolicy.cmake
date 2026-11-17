if(BUILD_TESTING)
    add_custom_target(native_required_owner_policy_tests)
    set(_halo2_required_policy_flags)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_required_policy_flags --sanitizers --semantic-mutants)
    endif()
    add_test(NAME native_tu15_required_owner_policy
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_required_owner_policy_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" ${_halo2_required_policy_flags}
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-required-owner-policy.json")
    set_tests_properties(native_tu15_required_owner_policy PROPERTIES TIMEOUT 300)
    add_test(NAME native_tu15_required_owner_policy_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_required_owner_policy_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-required-owner-policy-release.json")
    set_tests_properties(native_tu15_required_owner_policy_release PROPERTIES TIMEOUT 120)
endif()
