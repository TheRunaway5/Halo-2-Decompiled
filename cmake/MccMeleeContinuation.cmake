if(BUILD_TESTING AND TARGET native_melee_continuation_resources_tests)
    set(HALO2_MCC_MELEE_CONTINUATION_MAP "" CACHE FILEPATH
        "Explicit prepared MCC map for native melee continuation resource-owner qualification")
    if(NOT "${HALO2_MCC_MELEE_CONTINUATION_MAP}" STREQUAL "")
        add_test(NAME native_melee_continuation_resources_mcc COMMAND native_melee_continuation_resources_tests
            --mcc-only "${HALO2_MCC_MELEE_CONTINUATION_MAP}")
        set_tests_properties(native_melee_continuation_resources_mcc PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 120 LABELS "selected-mcc-native")
    endif()
endif()
