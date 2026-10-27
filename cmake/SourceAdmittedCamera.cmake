# Raw player-camera replay remains denied. Only the admitted numeric owner is
# available to the separately negotiated opt-in terminal stream.
target_sources(halo2_systems PRIVATE src/gameplay/vehicles/source_admitted_camera.cpp)
if(BUILD_TESTING)
    add_executable(source_admitted_camera_tests
        tests/source_admitted_camera_tests.cpp)
    target_link_libraries(source_admitted_camera_tests PRIVATE
        halo2_authority_codec nlohmann_json::nlohmann_json)
    add_test(NAME source_admitted_camera_tu15 COMMAND source_admitted_camera_tests
        "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}")
    set_tests_properties(source_admitted_camera_tu15 PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 180 RUN_SERIAL TRUE)
    if(TARGET source_authority_replay_tests)
        target_link_libraries(source_authority_replay_tests PRIVATE nlohmann_json::nlohmann_json)
        add_test(NAME source_admitted_camera_stream_http COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
        set_tests_properties(source_admitted_camera_stream_http PROPERTIES TIMEOUT 330 RUN_SERIAL TRUE
            ENVIRONMENT "HALO2_ADMITTED_CAMERA_STREAM=1;HALO2_RECEIVER_ASYNC=1;HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-admitted-camera-stream")
        if(EXISTS "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}")
            add_test(NAME source_admitted_camera_stream_mcc_http COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
            set_tests_properties(source_admitted_camera_stream_mcc_http PROPERTIES TIMEOUT 330 RUN_SERIAL TRUE
                ENVIRONMENT "HALO2_ADMITTED_CAMERA_STREAM=1;HALO2_RECEIVER_ASYNC=1;HALO2_SOURCE_AUTHORITY_MCC_MAP=${HALO2_MCC_MULTIPLAYER_ENGINE_MAP};HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-admitted-camera-stream-mcc")
            add_test(NAME source_admitted_camera_ongoing_mcc_http COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
            set_tests_properties(source_admitted_camera_ongoing_mcc_http PROPERTIES TIMEOUT 330 RUN_SERIAL TRUE
                ENVIRONMENT "HALO2_ADMITTED_CAMERA_STREAM=1;HALO2_ADMITTED_CAMERA_ONGOING=1;HALO2_RECEIVER_ASYNC=1;HALO2_SOURCE_AUTHORITY_MCC_MAP=${HALO2_MCC_MULTIPLAYER_ENGINE_MAP};HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-admitted-camera-ongoing-mcc")
        endif()
    endif()
endif()
