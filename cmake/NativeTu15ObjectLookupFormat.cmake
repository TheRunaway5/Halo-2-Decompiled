if(BUILD_TESTING)
    add_custom_target(native_object_lookup_format_tests)
    set(_halo2_lookup_format_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_lookup_format_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME native_tu15_object_lookup_format
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_lookup_format_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-lookup-format"
        --compiler "${CMAKE_CXX_COMPILER}" ${_halo2_lookup_format_sanitizers})
    set_tests_properties(native_tu15_object_lookup_format PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_object_lookup_format_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_lookup_format_tu15.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-lookup-format-release"
            --compiler "${CMAKE_CXX_COMPILER}")
        set_tests_properties(native_tu15_object_lookup_format_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_lookup_format_sanitizers)
endif()
