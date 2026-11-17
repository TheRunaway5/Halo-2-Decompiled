if(BUILD_TESTING)
    add_custom_target(native_object_function_query_tests)
    add_executable(native_object_function_query_public_tests tests/native_tu15_object_function_query_public_tests.cpp)
    target_include_directories(native_object_function_query_public_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(native_object_function_query_public_tests PRIVATE halo2_systems)
    add_dependencies(native_object_function_query_tests native_object_function_query_public_tests)
    set(_halo2_c9070_accounting)
    if(HALO2_NATIVE_EXECUTION_ACCOUNTING)
        list(APPEND _halo2_c9070_accounting --accounting)
    endif()
    add_test(NAME native_object_function_query_public
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_function_query_public_tu15.py"
        --binary "$<TARGET_FILE:native_object_function_query_public_tests>"
        --map "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/coagulation.map"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-function-query-public.json" --native-1046c0 ${_halo2_c9070_accounting})
    add_test(NAME native_object_function_query_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_function_query_binding_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-function-query-binding.json")
    set_tests_properties(native_object_function_query_public native_object_function_query_binding
        PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    unset(_halo2_c9070_accounting)
endif()
