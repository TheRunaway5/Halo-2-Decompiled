if(BUILD_TESTING)
    add_custom_target(native_object_body_lookup_value_tests)
    set(_halo2_object_body_value_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_object_body_value_sanitizers --sanitizers)
    endif()
    add_test(NAME native_tu15_object_body_lookup_value
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_value_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-value"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_object_body_value_sanitizers})
    set_tests_properties(native_tu15_object_body_lookup_value PROPERTIES TIMEOUT 600)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_object_body_lookup_value_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_value_tu15.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-value-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
        set_tests_properties(native_tu15_object_body_lookup_value_release PROPERTIES TIMEOUT 600)
    endif()
    unset(_halo2_object_body_value_sanitizers)
endif()
