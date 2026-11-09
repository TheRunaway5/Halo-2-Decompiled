if(BUILD_TESTING)
    add_custom_target(native_tu15_async_queue_data_tests)
    add_test(NAME native_tu15_async_queue_data_bindings
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_async_queue_data_bindings.py")
    set_tests_properties(native_tu15_async_queue_data_bindings PROPERTIES TIMEOUT 60)
    set(_halo2_async_queue_data_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_async_queue_data_sanitizers --sanitizers --leak-sanitizer)
    endif()
    add_test(NAME original_tu15_native_async_queue_data
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_async_queue_abi.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-async-queue-data.json"
        --require-binding --mutations ${_halo2_async_queue_data_sanitizers})
    set_tests_properties(original_tu15_native_async_queue_data PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    add_test(NAME native_tu15_async_queue_public_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_async_queue_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-async-queue-values.json"
        ${_halo2_async_queue_data_sanitizers})
    set_tests_properties(native_tu15_async_queue_public_values PROPERTIES TIMEOUT 300)
    add_test(NAME native_tu15_async_queue_source_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_async_queue_routes.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-async-queue-source-routes.json"
        ${_halo2_async_queue_data_sanitizers})
    set_tests_properties(native_tu15_async_queue_source_routes PROPERTIES TIMEOUT 300)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_async_queue_data_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_async_queue_abi.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-async-queue-data-release.json"
            --require-binding --mutations)
        set_tests_properties(original_tu15_native_async_queue_data_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_async_queue_data_sanitizers)
endif()
