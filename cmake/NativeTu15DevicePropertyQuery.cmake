if(BUILD_TESTING)
    add_custom_target(native_device_property_query_tests)
    set(_halo2_device_query_checks)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_device_query_checks --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_device_property_query_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_device_property_query_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-device-property-query-frame"
        --prefixes --observers ${_halo2_device_query_checks})
    add_test(NAME original_tu15_native_device_property_query_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_device_property_query_source_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-device-property-query-source"
        --require-binding ${_halo2_device_query_checks})
    add_test(NAME native_device_property_query_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_device_property_query_binding_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-device-property-query-binding" --require-binding)
    set_tests_properties(original_tu15_native_device_property_query_frame
        original_tu15_native_device_property_query_source native_device_property_query_binding
        PROPERTIES TIMEOUT 2400 SKIP_RETURN_CODE 77)
    if(TARGET native_map_world_property_queries_public_tests)
        add_test(NAME native_device_property_query_public
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_map_world_property_queries_public_tu15.py"
            --include-device --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --native "$<TARGET_FILE:native_map_world_property_queries_public_tests>"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-device-property-query-public")
        set_tests_properties(native_device_property_query_public PROPERTIES TIMEOUT 2400 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_device_query_checks)
endif()
