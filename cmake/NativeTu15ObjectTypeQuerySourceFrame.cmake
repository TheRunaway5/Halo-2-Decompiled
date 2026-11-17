if(BUILD_TESTING)
    add_custom_target(native_object_type_query_source_frame_tests)
    set(_halo2_type_query_source_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_type_query_source_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME native_tu15_object_type_query_source_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_type_query_source_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-type-query-source-frame"
        ${_halo2_type_query_source_sanitizers})
    set_tests_properties(native_tu15_object_type_query_source_frame PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_object_type_query_source_frame_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_type_query_source_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-type-query-source-frame-release")
        set_tests_properties(native_tu15_object_type_query_source_frame_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_type_query_source_sanitizers)
endif()
