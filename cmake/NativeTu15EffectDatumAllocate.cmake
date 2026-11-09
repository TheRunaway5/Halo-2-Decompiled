if(BUILD_TESTING)
    add_custom_target(native_effect_datum_allocate_tests)
    set(_halo2_effect_datum_allocate_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_effect_datum_allocate_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_effect_datum_allocate
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_effect_datum_allocate_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-effect-datum-allocate.json"
        --compiler "${CMAKE_CXX_COMPILER}"
        --require-binding ${_halo2_effect_datum_allocate_sanitizers})
    set_tests_properties(original_tu15_native_effect_datum_allocate PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    unset(_halo2_effect_datum_allocate_sanitizers)
endif()
