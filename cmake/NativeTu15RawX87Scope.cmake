if(BUILD_TESTING)
    add_custom_target(native_raw_x87_scope_tests)
    set(_halo2_raw_x87_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_raw_x87_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_raw_x87_scope
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_raw_x87_scope_tu15.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-raw-x87-scope.json
        --negative-controls ${_halo2_raw_x87_sanitizers})
    set_tests_properties(original_tu15_native_raw_x87_scope PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_raw_x87_scope_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_raw_x87_scope_tu15.py
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-raw-x87-scope-release.json)
        set_tests_properties(original_tu15_native_raw_x87_scope_release PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_raw_x87_sanitizers)
endif()
