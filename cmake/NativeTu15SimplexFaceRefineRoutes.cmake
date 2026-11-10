if(BUILD_TESTING)
    add_custom_target(native_simplex_face_refine_routes_tests)
    set(_halo2_simplex_face_refine_routes_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_simplex_face_refine_routes_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_simplex_face_refine_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_simplex_face_refine_routes_tu15.py"
        --require-binding
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-simplex-face-refine-routes.json"
        ${_halo2_simplex_face_refine_routes_sanitizers})
    set_tests_properties(original_tu15_native_simplex_face_refine_routes PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    unset(_halo2_simplex_face_refine_routes_sanitizers)
endif()
