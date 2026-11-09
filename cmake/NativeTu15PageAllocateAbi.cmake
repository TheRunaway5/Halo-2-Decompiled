if(BUILD_TESTING)
    add_custom_target(native_page_allocate_abi_tests)
    set(_halo2_page_allocate_abi_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_page_allocate_abi_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_page_allocate_abi
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_page_allocate_abi_tu15.py"
        --parent-io --canonical-io-diagnostic --faults --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-page-allocation-abi.json"
        ${_halo2_page_allocate_abi_sanitizers})
    set_tests_properties(original_tu15_native_page_allocate_abi PROPERTIES TIMEOUT 900)
    unset(_halo2_page_allocate_abi_sanitizers)
endif()
