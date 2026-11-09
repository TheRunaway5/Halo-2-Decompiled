if(BUILD_TESTING)
    add_custom_target(native_datum_specific_tests)
    set(_halo2_datum_specific_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_datum_specific_sanitizers --sanitizers)
    endif()
    foreach(_halo2_datum_part initialize specific)
        add_test(NAME original_tu15_native_datum_${_halo2_datum_part}
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_datum_${_halo2_datum_part}_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-datum-${_halo2_datum_part}.json"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_datum_specific_sanitizers})
        set_tests_properties(original_tu15_native_datum_${_halo2_datum_part} PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
        if(HALO2_SANITIZERS AND NOT MSVC)
            add_test(NAME original_tu15_native_datum_${_halo2_datum_part}_release
                COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_datum_${_halo2_datum_part}_tu15.py"
                --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
                --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-datum-${_halo2_datum_part}-release.json"
                --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
            set_tests_properties(original_tu15_native_datum_${_halo2_datum_part}_release PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
        endif()
    endforeach()
    unset(_halo2_datum_part)
    unset(_halo2_datum_specific_sanitizers)
endif()
