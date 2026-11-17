if(BUILD_TESTING)
    add_executable(native_indexed_datum_gate_linked_tests tests/native_tu15_indexed_datum_gate_linked_tests.cpp)
    target_include_directories(native_indexed_datum_gate_linked_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(native_indexed_datum_gate_linked_tests PRIVATE halo2_systems)

    set(_halo2_indexed_map_pack "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
    set(_halo2_indexed_accounting)
    if(HALO2_NATIVE_EXECUTION_ACCOUNTING)
        set(_halo2_indexed_accounting --accounting)
    endif()
    set(_halo2_indexed_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        set(_halo2_indexed_sanitizers --sanitizers)
    endif()

    add_test(NAME original_tu15_native_indexed_datum_gate_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_indexed_datum_gate_frame_tu15.py"
        --prefixes --observers --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-indexed-datum-gate-tu15" ${_halo2_indexed_sanitizers})
    add_test(NAME original_mappack11_native_indexed_datum_gate_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_indexed_datum_gate_frame_tu15.py"
        --mappack11 --prefixes --observers --compiler "${CMAKE_CXX_COMPILER}" --xbe "${_halo2_indexed_map_pack}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-indexed-datum-gate-mappack11" ${_halo2_indexed_sanitizers})
    add_test(NAME native_indexed_datum_gate_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_indexed_datum_gate_source_tu15.py"
        --require-binding --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --mappack "${_halo2_indexed_map_pack}" --output "${CMAKE_CURRENT_BINARY_DIR}/native-indexed-datum-gate-source"
        ${_halo2_indexed_sanitizers})
    add_test(NAME native_indexed_datum_gate_linked
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_indexed_datum_gate_linked_tu15.py"
        --binary "$<TARGET_FILE:native_indexed_datum_gate_linked_tests>" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --mappack "${_halo2_indexed_map_pack}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-indexed-datum-gate-linked.json"
        ${_halo2_indexed_accounting})
    set_tests_properties(original_tu15_native_indexed_datum_gate_frame original_mappack11_native_indexed_datum_gate_frame
        native_indexed_datum_gate_source native_indexed_datum_gate_linked PROPERTIES TIMEOUT 1500 SKIP_RETURN_CODE 77)
    unset(_halo2_indexed_sanitizers)
    unset(_halo2_indexed_accounting)
    unset(_halo2_indexed_map_pack)
endif()
