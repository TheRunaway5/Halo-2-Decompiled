if(BUILD_TESTING)
    add_custom_target(native_solver_hull_edge_routes_tests)
    set(_halo2_edge_routes_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_edge_routes_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_solver_hull_edge_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_edge_routes_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --mutations --require-binding
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-edge-routes.json"
        ${_halo2_edge_routes_sanitizers})
    # Eleven provider/publication matrices plus seven complete source mutant
    # builds need more than twenty minutes under concurrent compiler workloads.
    set_tests_properties(original_tu15_native_solver_hull_edge_routes PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
    unset(_halo2_edge_routes_sanitizers)
endif()
