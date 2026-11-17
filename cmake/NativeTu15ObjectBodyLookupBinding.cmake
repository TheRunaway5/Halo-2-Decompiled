if(BUILD_TESTING)
    add_custom_target(native_object_body_lookup_binding_tests)
    set(_halo2_object_body_binding_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_object_body_binding_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME native_tu15_object_body_lookup_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_routes_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-routes"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_object_body_binding_sanitizers})
    add_test(NAME native_tu15_object_body_lookup_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_source_generation_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-generation"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --candidate)
    set_tests_properties(native_tu15_object_body_lookup_routes native_tu15_object_body_lookup_generation
        PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_object_body_lookup_routes_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_routes_tu15.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-routes-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
        set_tests_properties(native_tu15_object_body_lookup_routes_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_object_body_binding_sanitizers)
endif()
