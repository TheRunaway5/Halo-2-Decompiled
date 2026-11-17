if(BUILD_TESTING)
    add_custom_target(native_mopp_diagnostic_return_tests)
    set(_halo2_mopp_diagnostic_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_mopp_diagnostic_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_mopp_diagnostic_return
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_diagnostic_return_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-mopp-diagnostic-return.json"
        ${_halo2_mopp_diagnostic_sanitizers})
    set_tests_properties(original_tu15_native_mopp_diagnostic_return PROPERTIES TIMEOUT 300)
endif()
