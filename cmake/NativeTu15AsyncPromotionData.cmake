if(BUILD_TESTING)
    add_custom_target(native_tu15_async_promotion_data_tests)
    add_test(NAME native_tu15_async_promotion_data_bindings
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_async_promotion_data_bindings.py")
    set_tests_properties(native_tu15_async_promotion_data_bindings PROPERTIES TIMEOUT 60)
    set(_halo2_async_promotion_data_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_async_promotion_data_sanitizers --sanitizers --leak-sanitizer)
    endif()
    # Unchanged CPU segments bracket genuine native wait/release operations.
    # No Xbox mutex/scheduler equivalence or supplied service outcome is used.
    add_test(NAME original_tu15_native_async_promotion_data
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_async_promotion_data.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-async-promotion-data.json"
        --require-binding --read-prefixes --source-routes --mutations ${_halo2_async_promotion_data_sanitizers})
    set_tests_properties(original_tu15_native_async_promotion_data PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    add_test(NAME native_tu15_async_promotion_public_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_async_promotion_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-async-promotion-values.json"
        --require-binding ${_halo2_async_promotion_data_sanitizers})
    set_tests_properties(native_tu15_async_promotion_public_values PROPERTIES TIMEOUT 300)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_async_promotion_data_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_async_promotion_data.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-async-promotion-data-release.json"
            --require-binding --read-prefixes --source-routes --mutations)
        set_tests_properties(original_tu15_native_async_promotion_data_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_async_promotion_data_sanitizers)
endif()
