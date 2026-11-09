if(BUILD_TESTING)
    add_custom_target(native_datum_delete_tests)
    set(_halo2_datum_delete_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_datum_delete_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_datum_delete
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_datum_delete_abi_tu15.py"
        --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-datum-delete-abi.json"
        ${_halo2_datum_delete_sanitizers})
    set_tests_properties(original_tu15_native_datum_delete PROPERTIES TIMEOUT 600)
    unset(_halo2_datum_delete_sanitizers)
endif()
