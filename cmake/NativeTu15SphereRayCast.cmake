if(BUILD_TESTING)
    add_custom_target(native_sphere_ray_cast_tests)
    set(_halo2_sphere_ray_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_sphere_ray_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_sphere_ray_cast
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sphere_ray_cast_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-sphere-ray-cast"
        ${_halo2_sphere_ray_sanitizers})
    set_tests_properties(original_tu15_native_sphere_ray_cast PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_sphere_ray_cast_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sphere_ray_cast_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-sphere-ray-cast-release")
        set_tests_properties(original_tu15_native_sphere_ray_cast_release PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_sphere_ray_sanitizers)
endif()
