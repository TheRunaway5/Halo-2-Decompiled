if(BUILD_TESTING)
    add_custom_target(native_sphere_ray_cast_routes_tests)
    set(_halo2_sphere_routes_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_sphere_routes_sanitizers --sanitizers --binding-mutants)
    endif()
    add_test(NAME original_tu15_native_sphere_ray_cast_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sphere_ray_cast_routes_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-sphere-ray-cast-routes"
        --require-binding ${_halo2_sphere_routes_sanitizers})
    set_tests_properties(original_tu15_native_sphere_ray_cast_routes PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_sphere_ray_cast_routes_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sphere_ray_cast_routes_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-sphere-ray-cast-routes-release"
            --require-binding)
        set_tests_properties(original_tu15_native_sphere_ray_cast_routes_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_sphere_routes_sanitizers)
endif()
