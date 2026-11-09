if(BUILD_TESTING)
    add_custom_target(native_polygon_features_tests)
    set(_halo2_polygon_features_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_polygon_features_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_polygon_features
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_polygon_features_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-polygon-features.json"
        ${_halo2_polygon_features_sanitizers})
    set_tests_properties(original_tu15_native_polygon_features PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_polygon_features_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_polygon_features_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-polygon-features-generation.json")
    set_tests_properties(original_tu15_native_polygon_features_generation PROPERTIES TIMEOUT 240)
    unset(_halo2_polygon_features_sanitizers)
endif()
