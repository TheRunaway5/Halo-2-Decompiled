target_sources(halo2_systems PRIVATE src/runtime/source_execution_campaign_screen.cpp)
if(BUILD_TESTING)
    add_executable(source_execution_campaign_screen_tests
        tests/source_execution_campaign_screen_tests.cpp
        src/runtime/source_execution_campaign_screen.cpp
        src/runtime/source_execution_inputs.cpp
        src/runtime/source_runtime_memory.cpp
        src/campaign/native_campaign_screen_lifecycle_tu15.cpp)
    target_include_directories(source_execution_campaign_screen_tests PRIVATE include)
    target_link_libraries(source_execution_campaign_screen_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    add_test(NAME native_campaign_screen_execution_journal COMMAND source_execution_campaign_screen_tests)
endif()
