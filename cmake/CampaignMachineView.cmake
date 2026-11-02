if(BUILD_TESTING)
    add_executable(source_campaign_machine_view_tests tests/source_campaign_machine_view_tests.cpp)
    target_link_libraries(source_campaign_machine_view_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    set(HALO2_CAMPAIGN_MACHINE_VIEW_MAP "" CACHE FILEPATH "Actual MCC Armory map for campaign machine resource regression")
    add_test(NAME source_campaign_machine_view_resources COMMAND source_campaign_machine_view_tests
        "${HALO2_CAMPAIGN_MACHINE_VIEW_MAP}")
    set_tests_properties(source_campaign_machine_view_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
