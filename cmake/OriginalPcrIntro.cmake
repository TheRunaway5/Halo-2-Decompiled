if(BUILD_TESTING)
    add_executable(original_pcr_intro_tests tests/original_pcr_intro_tests.cpp)
    target_include_directories(original_pcr_intro_tests PRIVATE include)
    target_link_libraries(original_pcr_intro_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_pcr_intro_kernel COMMAND original_pcr_intro_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_pcr_intro_original.json")
    set_tests_properties(original_pcr_intro_kernel PROPERTIES TIMEOUT 30)
    add_executable(original_ui_map_intro_tests tests/original_ui_map_intro_tests.cpp)
    target_include_directories(original_ui_map_intro_tests PRIVATE include)
    target_link_libraries(original_ui_map_intro_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_ui_map_intro_kernel COMMAND original_ui_map_intro_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_ui_map_intro_original.json")
    set_tests_properties(original_ui_map_intro_kernel PROPERTIES TIMEOUT 30)
    if(HALO2_MCC_PARTY_UI_TEST_CACHE)
        add_test(NAME mcc_original_pcr_intro_gpu COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_pcr_intro_gpu_test.py"
            --binary $<TARGET_FILE:frontend_vista_pcr_painter_tests>
            --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_results_original.tsv")
        set_tests_properties(mcc_original_pcr_intro_gpu PROPERTIES SKIP_RETURN_CODE 77
            TIMEOUT 90 RUN_SERIAL TRUE)
    endif()
endif()
