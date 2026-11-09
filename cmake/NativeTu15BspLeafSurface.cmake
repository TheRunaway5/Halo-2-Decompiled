if(BUILD_TESTING)
    add_custom_target(native_bsp_leaf_surface_tests)
    set(_halo2_bsp_leaf_surface_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_bsp_leaf_surface_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_bsp_leaf_surface
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_leaf_surface_abi_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-leaf-surface.json"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_bsp_leaf_surface_sanitizers})
    set_tests_properties(original_tu15_native_bsp_leaf_surface PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_bsp_leaf_surface_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_leaf_surface_abi_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-leaf-surface-release.json"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
        set_tests_properties(original_tu15_native_bsp_leaf_surface_release PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_bsp_leaf_surface_sanitizers)
endif()
