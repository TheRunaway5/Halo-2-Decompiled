if(BUILD_TESTING)
    add_custom_target(native_physical_callbacks_tests)
    set(_halo2_physical_callbacks_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_physical_callbacks_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_physical_callbacks
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_physical_callbacks_abi_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-physical-callbacks-abi.json"
        ${_halo2_physical_callbacks_sanitizers})
    set_tests_properties(original_tu15_native_physical_callbacks PROPERTIES TIMEOUT 240)
    unset(_halo2_physical_callbacks_sanitizers)
endif()
