# Qualify real MCC body actions without an external gameplay executable.
# No automatic global cache selection or skip acceptance for requested inputs.
if(BUILD_TESTING)
    add_executable(mcc_body_action_tests tests/mcc_body_action_tests.cpp)
    target_link_libraries(mcc_body_action_tests PRIVATE halo2_systems OpenSSL::Crypto)
    set(HALO2_MCC_BODY_ANIMATION_TEST_MAP "" CACHE FILEPATH
        "Explicit prepared MCC map for actual BODY selector/action qualification")
    if(NOT "${HALO2_MCC_BODY_ANIMATION_TEST_MAP}" STREQUAL "")
        if(NOT IS_ABSOLUTE "${HALO2_MCC_BODY_ANIMATION_TEST_MAP}" OR
            NOT EXISTS "${HALO2_MCC_BODY_ANIMATION_TEST_MAP}" OR
            IS_DIRECTORY "${HALO2_MCC_BODY_ANIMATION_TEST_MAP}")
            message(FATAL_ERROR "HALO2_MCC_BODY_ANIMATION_TEST_MAP requires an existing absolute prepared MCC map")
        endif()
        add_test(NAME native_mcc_body_actions COMMAND mcc_body_action_tests
            "${HALO2_MCC_BODY_ANIMATION_TEST_MAP}")
        set_tests_properties(native_mcc_body_actions PROPERTIES
            TIMEOUT 120 RUN_SERIAL TRUE
            LABELS "genuine-mcc-input;selected-mcc-native;animation-selector-component")
    endif()
endif()
