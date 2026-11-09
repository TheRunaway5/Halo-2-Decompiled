if(BUILD_TESTING)
    add_custom_target(native_page_window_tests)
    set(_halo2_page_window_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_page_window_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_page_window
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_page_window_abi_tu15.py"
        --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-page-window-abi.json"
        ${_halo2_page_window_sanitizers})
    set_tests_properties(original_tu15_native_page_window PROPERTIES TIMEOUT 240)
    unset(_halo2_page_window_sanitizers)
endif()
