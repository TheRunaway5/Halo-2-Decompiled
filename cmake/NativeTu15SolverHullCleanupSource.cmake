if(BUILD_TESTING)
    add_custom_target(native_solver_hull_cleanup_source_tests)
    set(_halo2_hull_cleanup_source_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_hull_cleanup_source_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_solver_hull_cleanup_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_cleanup_source_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-cleanup-source.json"
        ${_halo2_hull_cleanup_source_sanitizers})
    set_tests_properties(original_tu15_native_solver_hull_cleanup_source
        PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    unset(_halo2_hull_cleanup_source_sanitizers)
endif()
