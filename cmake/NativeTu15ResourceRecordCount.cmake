if(BUILD_TESTING)
    add_custom_target(native_resource_record_count_tests)
    set(_halo2_resource_record_count_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_resource_record_count_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_resource_record_count
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_resource_record_count.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-resource-record-count.json"
        --require-binding ${_halo2_resource_record_count_sanitizers})
    set_tests_properties(original_tu15_native_resource_record_count PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    unset(_halo2_resource_record_count_sanitizers)
endif()
