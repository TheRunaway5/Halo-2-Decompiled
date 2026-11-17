if(BUILD_TESTING)
    add_custom_target(native_solver_hull_line_frame_tests)
    set(_halo2_hull_line_frame_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_hull_line_frame_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_solver_hull_line_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_line_frame_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --hardware --prefixes --mutations
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-line-frame.json"
        ${_halo2_hull_line_frame_sanitizers})
    set_tests_properties(original_tu15_native_solver_hull_line_frame PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    unset(_halo2_hull_line_frame_sanitizers)
endif()
