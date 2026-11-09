if(BUILD_TESTING)
    add_custom_target(native_sphere_frustum_tests)
    set(_halo2_sphere_frustum_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_sphere_frustum_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_sphere_frustum
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sphere_frustum_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-sphere-frustum.json"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --mmx ${_halo2_sphere_frustum_sanitizers})
    set_tests_properties(original_tu15_native_sphere_frustum PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_sphere_frustum_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sphere_frustum_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-sphere-frustum-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}")
    set_tests_properties(original_tu15_native_sphere_frustum_generation PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_sphere_frustum_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sphere_frustum_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-sphere-frustum-release.json"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding --mmx)
        set_tests_properties(original_tu15_native_sphere_frustum_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_sphere_frustum_sanitizers)
endif()
