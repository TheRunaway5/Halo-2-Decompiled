if(BUILD_TESTING)
    add_custom_target(native_object_body_lookup_frame_tests)
    set(_halo2_object_body_lookup_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_object_body_lookup_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_object_body_lookup_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-frame"
        --prefixes --source-publication ${_halo2_object_body_lookup_sanitizers})
    set_tests_properties(original_tu15_native_object_body_lookup_frame PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_object_body_lookup_frame_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-frame-release"
            --prefixes --source-publication)
        set_tests_properties(original_tu15_native_object_body_lookup_frame_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_object_body_lookup_sanitizers)
endif()
