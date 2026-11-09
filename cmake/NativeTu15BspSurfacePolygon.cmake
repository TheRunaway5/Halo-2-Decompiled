if(BUILD_TESTING)
    add_custom_target(native_bsp_surface_polygon_tests)
    set(_halo2_bsp_polygon_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_bsp_polygon_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_bsp_surface_polygon
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_surface_polygon_abi_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-surface-polygon.json"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_bsp_polygon_sanitizers})
    set_tests_properties(original_tu15_native_bsp_surface_polygon PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    unset(_halo2_bsp_polygon_sanitizers)
endif()
