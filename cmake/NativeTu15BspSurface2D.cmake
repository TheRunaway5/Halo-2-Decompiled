if(BUILD_TESTING)
    add_custom_target(native_bsp_surface_2d_tests)
    set(_halo2_bsp_surface_2d_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_bsp_surface_2d_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_bsp_surface_2d
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_surface_2d_abi_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-surface-2d.json"
        --require-binding ${_halo2_bsp_surface_2d_sanitizers})
    set_tests_properties(original_tu15_native_bsp_surface_2d PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_bsp_surface_2d_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_surface_2d_abi_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-surface-2d-release.json"
            --require-binding)
        set_tests_properties(original_tu15_native_bsp_surface_2d_release PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_bsp_surface_2d_sanitizers)
endif()
