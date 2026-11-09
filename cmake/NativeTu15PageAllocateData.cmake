if(BUILD_TESTING)
    add_custom_target(native_page_allocate_data_tests)
    set(_halo2_page_allocate_data_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_page_allocate_data_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_page_allocate_data
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_page_allocate_data_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-page-allocation-data.json"
        ${_halo2_page_allocate_data_sanitizers})
    set_tests_properties(original_tu15_native_page_allocate_data PROPERTIES TIMEOUT 240)
    unset(_halo2_page_allocate_data_sanitizers)
endif()
