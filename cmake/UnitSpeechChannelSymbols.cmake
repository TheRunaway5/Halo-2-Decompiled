if(BUILD_TESTING)
    add_executable(source_unit_speech_channel_symbols_tu15_tests
        tests/source_unit_speech_channel_symbols_tu15_tests.cpp
        tests/source_unit_speech_channel_symbols_tu15_peer.cpp)
    target_include_directories(source_unit_speech_channel_symbols_tu15_tests PRIVATE include)
    target_link_libraries(source_unit_speech_channel_symbols_tu15_tests PRIVATE Threads::Threads)
    add_test(NAME source_unit_speech_channel_symbols_tu15
        COMMAND source_unit_speech_channel_symbols_tu15_tests)
endif()
