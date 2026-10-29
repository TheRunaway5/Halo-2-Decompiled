if(BUILD_TESTING AND TARGET halo2_cache)
    add_executable(native_campaign_ai_fraction_layout_tu15_tests
        tests/native_campaign_ai_fraction_layout_tu15_tests.cpp)
    target_link_libraries(native_campaign_ai_fraction_layout_tu15_tests PRIVATE halo2_cache)
    add_test(NAME native_campaign_ai_fraction_layout_tu15
        COMMAND native_campaign_ai_fraction_layout_tu15_tests)
endif()
