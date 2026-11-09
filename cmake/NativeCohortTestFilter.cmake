if(BUILD_TESTING)
    add_custom_target(native_cohort_test_filter_tests)
    add_test(NAME native_replacement_cohort_filter
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_cohort_test_filter.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-cohort-test-filter.json")
    set_tests_properties(native_replacement_cohort_filter PROPERTIES TIMEOUT 60)
endif()
