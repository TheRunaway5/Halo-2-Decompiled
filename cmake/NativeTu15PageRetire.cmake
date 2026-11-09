if(BUILD_TESTING)
    add_custom_target(native_page_retire_tests)
    set(_halo2_page_retire_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_page_retire_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_page_retire
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_page_retire_abi_tu15.py"
        --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-page-retire-abi.json"
        ${_halo2_page_retire_sanitizers})
    set_tests_properties(original_tu15_native_page_retire PROPERTIES TIMEOUT 600)
    unset(_halo2_page_retire_sanitizers)
endif()
