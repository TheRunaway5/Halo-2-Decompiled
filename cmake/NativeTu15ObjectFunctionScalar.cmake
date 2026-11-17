if(BUILD_TESTING)
    add_custom_target(native_object_function_scalar_frame_tests)
    add_custom_target(native_object_function_scalar_tests)
    add_dependencies(native_object_function_scalar_tests
        native_object_function_scalar_frame_tests object_function_symbol_tu15_tests)
    set(_halo2_bcb70_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_bcb70_sanitizers --sanitizers)
    endif()
    set(_halo2_bcb70_accounting)
    if(HALO2_NATIVE_EXECUTION_ACCOUNTING)
        list(APPEND _halo2_bcb70_accounting --accounting)
    endif()
    add_test(NAME original_tu15_native_object_function_scalar_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_function_scalar_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-function-scalar-frame.json" ${_halo2_bcb70_sanitizers})
    add_test(NAME native_object_function_scalar_producer
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_function_scalar_producer_tu15.py"
        --binary "$<TARGET_FILE:object_function_symbol_tu15_tests>" --map "${HALO2_OBJECT_FUNCTION_MCC_MAP}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-function-scalar-producer.json" ${_halo2_bcb70_accounting})
    set_tests_properties(original_tu15_native_object_function_scalar_frame native_object_function_scalar_producer
        PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    unset(_halo2_bcb70_sanitizers)
    unset(_halo2_bcb70_accounting)
endif()
