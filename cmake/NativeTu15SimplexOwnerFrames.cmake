if(BUILD_TESTING)
    add_custom_target(native_simplex_owner_frame_tests)
    set(_halo2_simplex_owner_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_simplex_owner_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_simplex_hull_owner_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_simplex_hull_owner_frame_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-simplex-hull-owner-frame.json"
        ${_halo2_simplex_owner_sanitizers})
    add_test(NAME original_tu15_native_simplex_owner_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_simplex_owner_frame_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-simplex-owner-frame.json"
        ${_halo2_simplex_owner_sanitizers})
    set_tests_properties(original_tu15_native_simplex_hull_owner_frame original_tu15_native_simplex_owner_frame
        PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    unset(_halo2_simplex_owner_sanitizers)
endif()
