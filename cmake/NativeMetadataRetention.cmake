if(BUILD_TESTING)
    add_custom_target(native_metadata_retention_tests)
    add_test(NAME native_metadata_storage
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_metadata.py")
    set_tests_properties(native_metadata_storage PROPERTIES TIMEOUT 120)
    add_test(NAME native_metadata_retention_replay
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_metadata_replay.py"
        --evidence-root "${HALO2_TEST_ASSET_SOURCE_ROOT}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-metadata-retention.json")
    set_tests_properties(native_metadata_retention_replay PROPERTIES TIMEOUT 180)
endif()
