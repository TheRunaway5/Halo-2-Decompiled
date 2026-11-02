if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_sound_class_read_order_tests
        tests/native_sound_class_read_order_tests.cpp
        tests/native_campaign_audio_stop_manager_groups.cpp
        src/audio/native_tu15_sound_manager.cpp)
    target_include_directories(native_sound_class_read_order_tests PRIVATE include src)
    target_link_libraries(native_sound_class_read_order_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(native_sound_class_read_order_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    add_test(NAME native_sound_class_read_order
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_native_sound_class_order_tu15.py
            --native $<TARGET_FILE:native_sound_class_read_order_tests>
            --fixture ${PROJECT_SOURCE_DIR}/tests/fixtures/native_sound_class_read_order_tu15.json.gz)
    set_tests_properties(native_sound_class_read_order PROPERTIES TIMEOUT 90)
endif()
