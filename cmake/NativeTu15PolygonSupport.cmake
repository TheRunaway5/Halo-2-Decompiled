if(BUILD_TESTING)
    add_custom_target(native_polygon_support_tests)
    set(_halo2_polygon_support_sanitizers)
    set(_halo2_polygon_legacy_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_polygon_support_sanitizers --sanitizers --mutations)
        list(APPEND _halo2_polygon_legacy_sanitizers --sanitizers --mutations)
    endif()
    add_test(NAME original_tu15_native_polygon_support
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_polygon_support_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-polygon-support"
        --source-routes --require-binding ${_halo2_polygon_support_sanitizers})
    set_tests_properties(original_tu15_native_polygon_support PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_polygon_support_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_polygon_support_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-polygon-support-release"
            --source-routes --require-binding)
        set_tests_properties(original_tu15_native_polygon_support_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    add_test(NAME original_tu15_native_polygon_support_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_polygon_support_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-polygon-support-generation.json")
    set_tests_properties(original_tu15_native_polygon_support_generation PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    add_test(NAME original_mappack_native_polygon_support
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_polygon_support_frame_tu11.py"
        --xbe "${HALO2_MAPPACK_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-polygon-support-mappack"
        --require-binding ${_halo2_polygon_legacy_sanitizers})
    set_tests_properties(original_mappack_native_polygon_support PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_mappack_native_polygon_support_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_polygon_support_frame_tu11.py"
            --xbe "${HALO2_MAPPACK_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-polygon-support-mappack-release"
            --require-binding)
        set_tests_properties(original_mappack_native_polygon_support_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_polygon_legacy_sanitizers)
    unset(_halo2_polygon_support_sanitizers)
endif()
