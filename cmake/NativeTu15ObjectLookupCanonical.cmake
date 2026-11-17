if(BUILD_TESTING)
    add_custom_target(native_object_lookup_canonical_tests)
    set(_halo2_canonical_lookup_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_canonical_lookup_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME native_tu15_object_lookup_canonical_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_lookup_canonical_routes.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-lookup-canonical-routes"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_canonical_lookup_sanitizers})
    add_test(NAME native_tu15_object_lookup_canonical_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_object_lookup_canonical_owners.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-lookup-canonical-generation"
        --candidate --require-binding)
    set_tests_properties(native_tu15_object_lookup_canonical_routes native_tu15_object_lookup_canonical_generation
        PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_object_lookup_canonical_routes_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_lookup_canonical_routes.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-lookup-canonical-routes-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
        set_tests_properties(native_tu15_object_lookup_canonical_routes_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_canonical_lookup_sanitizers)
endif()
