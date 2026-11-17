if(BUILD_TESTING)
    option(HALO2_NATIVE_OBJECT_TYPE_QUERY_REQUIRE_BINDING "Require installed TypeQuery parent defaults" ON)
    set(_halo2_type_query_owner_binding)
    if(HALO2_NATIVE_OBJECT_TYPE_QUERY_REQUIRE_BINDING)
        list(APPEND _halo2_type_query_owner_binding --require-binding)
    endif()
    add_custom_target(native_object_type_query_owner_tests)
    set(_halo2_type_query_owner_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_type_query_owner_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME native_tu15_object_type_query_owner
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_type_query_owner_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-type-query-owner"
        ${_halo2_type_query_owner_sanitizers})
    add_test(NAME native_tu15_object_type_query_candidates
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_object_type_query_candidates.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-type-query-candidates"
        ${_halo2_type_query_owner_binding})
    add_test(NAME native_tu15_object_type_query_candidate_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_type_query_candidates_routes.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-type-query-candidate-routes"
        ${_halo2_type_query_owner_binding} ${_halo2_type_query_owner_sanitizers})
    set_tests_properties(native_tu15_object_type_query_owner native_tu15_object_type_query_candidates native_tu15_object_type_query_candidate_routes
        PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_object_type_query_owner_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_type_query_owner_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-type-query-owner-release")
        set_tests_properties(native_tu15_object_type_query_owner_release
            PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
        add_test(NAME native_tu15_object_type_query_candidate_routes_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_type_query_candidates_routes.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-type-query-candidate-routes-release"
            ${_halo2_type_query_owner_binding})
        set_tests_properties(native_tu15_object_type_query_candidate_routes_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_type_query_owner_sanitizers)
    unset(_halo2_type_query_owner_binding)
endif()
