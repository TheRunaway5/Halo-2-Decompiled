if(BUILD_TESTING)
    add_custom_target(native_object_table_ratio_tests)
    set(_halo2_table_ratio_checks)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_table_ratio_checks --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_object_table_ratio_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_table_ratio_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-table-ratio-frame"
        --prefixes ${_halo2_table_ratio_checks})
    add_test(NAME original_tu15_native_object_table_ratio_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_table_ratio_source_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-table-ratio-source"
        --require-binding ${_halo2_table_ratio_checks})
    add_test(NAME native_object_table_ratio_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_table_ratio_binding_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-table-ratio-binding" --require-binding)
    set_tests_properties(original_tu15_native_object_table_ratio_frame
        original_tu15_native_object_table_ratio_source native_object_table_ratio_binding
        PROPERTIES TIMEOUT 2400 SKIP_RETURN_CODE 77)
    add_executable(native_object_table_ratio_public_tests tests/native_tu15_object_table_ratio_public_tests.cpp)
    target_include_directories(native_object_table_ratio_public_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(native_object_table_ratio_public_tests PRIVATE halo2_physics_world_runtime)
    add_dependencies(native_object_table_ratio_tests native_object_table_ratio_public_tests)
    add_test(NAME native_object_table_ratio_public
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_table_ratio_public_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native "$<TARGET_FILE:native_object_table_ratio_public_tests>"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-table-ratio-public")
    set_tests_properties(native_object_table_ratio_public PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    unset(_halo2_table_ratio_checks)
endif()
