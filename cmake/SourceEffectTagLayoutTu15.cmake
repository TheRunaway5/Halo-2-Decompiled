target_sources(halo2_cache PRIVATE src/source_effect_tag_layout_tu15.cpp)
if(BUILD_TESTING)
    add_executable(source_effect_tag_layout_tu15_tests
        tests/source_effect_tag_layout_tu15_tests.cpp src/source_effect_tag_layout_tu15.cpp)
    target_include_directories(source_effect_tag_layout_tu15_tests PRIVATE include)
    add_test(NAME source_effect_tag_layout_tu15 COMMAND source_effect_tag_layout_tu15_tests)
    add_executable(source_effect_tag_layout_tu15_mcc_tests tests/source_effect_tag_layout_tu15_mcc_tests.cpp)
    target_link_libraries(source_effect_tag_layout_tu15_mcc_tests PRIVATE halo2_cache)
    add_test(NAME mcc_source_effect_tag_layout_tu15 COMMAND source_effect_tag_layout_tu15_mcc_tests
        "${HALO2_MCC_SOURCE_TAG_DONOR}")
    set_tests_properties(mcc_source_effect_tag_layout_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
