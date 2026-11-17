if(BUILD_TESTING)
    add_custom_target(native_solver_hull_support_pair_tests)
    set(_halo2_solver_hull_support_pair_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_solver_hull_support_pair_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_solver_hull_support_pair
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_support_pair_original_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --mutations
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-support-pair.json"
        ${_halo2_solver_hull_support_pair_sanitizers})
    set_tests_properties(original_tu15_native_solver_hull_support_pair PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    unset(_halo2_solver_hull_support_pair_sanitizers)
endif()
