target_sources(halo2_systems PRIVATE src/native_tu15_observer_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_observer_data_tests tests/native_tu15_observer_data_tests.cpp)
    target_link_libraries(native_tu15_observer_data_tests PRIVATE halo2_systems)
    add_test(NAME original_native_tu15_observer_loader_data
        COMMAND native_tu15_observer_data_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_observer_data.json)
    set_tests_properties(original_native_tu15_observer_loader_data PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 60 RUN_SERIAL TRUE)
endif()
