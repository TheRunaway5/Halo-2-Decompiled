if(BUILD_TESTING)
    add_custom_target(native_node_progress_query_tests)
    set(_halo2_node_progress_sanitizers)
    set(_halo2_node_progress_defects)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_node_progress_sanitizers --sanitizers)
        list(APPEND _halo2_node_progress_defects --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_node_progress_query_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_node_progress_query_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-node-progress-query-frame"
        --prefixes ${_halo2_node_progress_sanitizers})
    add_test(NAME original_tu15_native_node_progress_query_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_node_progress_query_source_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-node-progress-query-source"
        --require-binding ${_halo2_node_progress_sanitizers} ${_halo2_node_progress_defects})
    add_test(NAME native_node_progress_query_local_caller
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_node_progress_query_source_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-node-progress-query-local"
        --local-parent ${_halo2_node_progress_sanitizers})
    add_test(NAME native_node_progress_query_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_node_progress_query_binding_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-node-progress-query-binding")
    set_tests_properties(original_tu15_native_node_progress_query_frame original_tu15_native_node_progress_query_source
        native_node_progress_query_local_caller native_node_progress_query_binding
        PROPERTIES TIMEOUT 2400 SKIP_RETURN_CODE 77)
    add_executable(native_d3950_world_admission_tests tests/native_d3950_world_admission_tests.cpp)
    target_link_libraries(native_d3950_world_admission_tests PRIVATE halo2_systems)
    add_test(NAME native_d3950_world_admission
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_d3950_world_admission_tu15.py"
        --binary "$<TARGET_FILE:native_d3950_world_admission_tests>"
        --map "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/coagulation.map"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-d3950-world-admission.json")
    set_tests_properties(native_d3950_world_admission PROPERTIES TIMEOUT 420 SKIP_RETURN_CODE 77)
    unset(_halo2_node_progress_sanitizers)
    unset(_halo2_node_progress_defects)
endif()
