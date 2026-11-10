if(BUILD_TESTING)
    add_custom_target(native_segment_sphere_tests)
    set(_halo2_segment_sphere_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_segment_sphere_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_segment_sphere
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_segment_sphere_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-segment-sphere"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies --mmx ${_halo2_segment_sphere_sanitizers})
    set_tests_properties(original_tu15_native_segment_sphere PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_segment_sphere_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_segment_sphere_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-segment-sphere-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}")
    set_tests_properties(original_tu15_native_segment_sphere_generation PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_segment_sphere_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_segment_sphere_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-segment-sphere-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies --mmx)
        set_tests_properties(original_tu15_native_segment_sphere_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_segment_sphere_sanitizers)
endif()
