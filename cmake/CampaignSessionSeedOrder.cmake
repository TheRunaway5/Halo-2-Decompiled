if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(campaign_session_seed_order_tu15_tests tests/campaign_session_seed_order_tu15_tests.cpp)
    target_include_directories(campaign_session_seed_order_tu15_tests PRIVATE include)
    target_link_libraries(campaign_session_seed_order_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    set(HALO2_CAMPAIGN_SEED_ORDER_MCC_MAP "" CACHE FILEPATH "Prepared MCC campaign map for original session-seed acceptance")
    if(EXISTS "${HALO2_CAMPAIGN_SEED_ORDER_MCC_MAP}")
        get_filename_component(campaign_seed_order_maps "${HALO2_CAMPAIGN_SEED_ORDER_MCC_MAP}" DIRECTORY)
        foreach(seed 305419896 2271560481)
            add_test(NAME native_campaign_session_seed_order_${seed}
                COMMAND campaign_session_seed_order_tu15_tests "${HALO2_CAMPAIGN_SEED_ORDER_MCC_MAP}"
                    "${campaign_seed_order_maps}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_session_seed_order_tu15.json"
                    "${seed}" "${CMAKE_CURRENT_BINARY_DIR}/campaign-session-seed-${seed}.json")
            set_tests_properties(native_campaign_session_seed_order_${seed} PROPERTIES TIMEOUT 360)
        endforeach()
    endif()
endif()
