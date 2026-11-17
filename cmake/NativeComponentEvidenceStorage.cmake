if(BUILD_TESTING)
    add_custom_target(native_component_evidence_storage_tests)
    add_test(NAME native_component_evidence_storage
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_component_evidence_storage.py")
    set_tests_properties(native_component_evidence_storage PROPERTIES TIMEOUT 120)
    add_test(NAME native_component_evidence_storage_replay
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_component_evidence_storage.py"
        --evidence-root "${HALO2_TEST_ASSET_SOURCE_ROOT}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-component-evidence-storage.json")
    set_tests_properties(native_component_evidence_storage_replay PROPERTIES TIMEOUT 180)
endif()
