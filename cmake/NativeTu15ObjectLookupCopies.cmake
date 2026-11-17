if(BUILD_TESTING)
    add_custom_target(native_object_lookup_copies_tests)
    set(_halo2_lookup_copies_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_lookup_copies_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME native_tu15_object_lookup_copies_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_lookup_copies_routes.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-lookup-copies-routes"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_lookup_copies_sanitizers})
    add_test(NAME native_tu15_object_lookup_copies_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_object_lookup_copies.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-lookup-copies-generation"
        --candidate --require-binding)
    set_tests_properties(native_tu15_object_lookup_copies_routes native_tu15_object_lookup_copies_generation
        PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_object_lookup_copies_routes_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_lookup_copies_routes.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-lookup-copies-routes-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
        set_tests_properties(native_tu15_object_lookup_copies_routes_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_lookup_copies_sanitizers)
endif()
