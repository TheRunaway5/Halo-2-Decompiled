if(BUILD_TESTING AND NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64)$")
    add_executable(campaign_model_skin_handoff_tests tests/campaign_model_skin_handoff_tests.cpp)
    target_include_directories(campaign_model_skin_handoff_tests PRIVATE include src)
    target_compile_options(campaign_model_skin_handoff_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(campaign_model_skin_handoff_tests PRIVATE halo2_systems Vulkan::Vulkan)
    set(HALO2_CAMPAIGN_SKIN_HANDOFF_MCC_MAP "" CACHE FILEPATH "Explicit prepared MCC Cairo map for campaign ranged skin handoff")
    if(NOT "${HALO2_CAMPAIGN_SKIN_HANDOFF_MCC_MAP}" STREQUAL "")
        if(NOT IS_ABSOLUTE "${HALO2_CAMPAIGN_SKIN_HANDOFF_MCC_MAP}" OR
            NOT EXISTS "${HALO2_CAMPAIGN_SKIN_HANDOFF_MCC_MAP}" OR IS_DIRECTORY "${HALO2_CAMPAIGN_SKIN_HANDOFF_MCC_MAP}")
            message(FATAL_ERROR "HALO2_CAMPAIGN_SKIN_HANDOFF_MCC_MAP requires an existing absolute prepared MCC Cairo map")
        endif()
        add_test(NAME native_mcc_campaign_model_skin_handoff COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- "$<TARGET_FILE:campaign_model_skin_handoff_tests>"
            "${HALO2_CAMPAIGN_SKIN_HANDOFF_MCC_MAP}")
        set_tests_properties(native_mcc_campaign_model_skin_handoff PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE
            LABELS "genuine-mcc-input;selected-mcc-native;campaign-gpu-skin-handoff-component")
    endif()
endif()
