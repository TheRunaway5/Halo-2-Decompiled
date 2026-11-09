if(BUILD_TESTING)
    add_custom_target(native_physical_epoch_tests)
    set(_halo2_physical_epoch_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_physical_epoch_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_physical_epoch
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_physical_epoch_abi_tu15.py"
        --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-physical-epoch-abi.json"
        ${_halo2_physical_epoch_sanitizers})
    set_tests_properties(original_tu15_native_physical_epoch PROPERTIES TIMEOUT 240)
    unset(_halo2_physical_epoch_sanitizers)
endif()
