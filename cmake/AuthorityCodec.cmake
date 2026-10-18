find_package(ZLIB 1.2.9 REQUIRED)
add_library(halo2_authority_delta STATIC src/source_authority_delta.cpp)
target_include_directories(halo2_authority_delta PUBLIC include)
target_compile_features(halo2_authority_delta PUBLIC cxx_std_20)
target_link_libraries(halo2_authority_delta PRIVATE OpenSSL::Crypto ZLIB::ZLIB)
target_sources(halo2_systems PRIVATE src/source_authority_binding.cpp)
target_link_libraries(halo2_systems PRIVATE OpenSSL::Crypto)
add_library(halo2_authority_codec STATIC src/source_authority_codec.cpp src/source_execution_codec.cpp src/source_replay_batch.cpp)
target_sources(halo2_authority_codec PRIVATE src/native_vehicle_camera_codec.cpp src/source_host_continuation_codec.cpp)
target_sources(halo2_authority_codec PRIVATE src/source_host_continuation_bundle.cpp)
target_compile_features(halo2_authority_codec PUBLIC cxx_std_20)
target_include_directories(halo2_authority_codec PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost-pfr/include")
target_link_libraries(halo2_authority_codec PUBLIC halo2_systems PRIVATE OpenSSL::Crypto ZLIB::ZLIB)
add_library(halo2_authority_publisher STATIC src/source_authority_publisher.cpp)
target_compile_features(halo2_authority_publisher PUBLIC cxx_std_20)
target_include_directories(halo2_authority_publisher PUBLIC include)
target_link_libraries(halo2_authority_publisher PUBLIC halo2_authority_codec PRIVATE halo2_authority_delta Threads::Threads)
add_library(halo2_authority_receiver STATIC src/source_authority_receiver.cpp)
target_sources(halo2_authority_receiver PRIVATE src/source_peer_host_takeover.cpp)
target_sources(halo2_authority_receiver PRIVATE src/multiplayer_runtime_authority.cpp)
target_compile_features(halo2_authority_receiver PUBLIC cxx_std_20)
target_include_directories(halo2_authority_receiver PUBLIC include)
target_link_libraries(halo2_authority_receiver PUBLIC halo2_authority_codec PRIVATE halo2_authority_publisher halo2_authority_delta Threads::Threads)
target_link_libraries(halo2_renderer PUBLIC halo2_authority_receiver halo2_authority_publisher)
if(BUILD_TESTING)
    add_executable(source_peer_host_takeover_tests tests/source_peer_host_takeover_tests.cpp)
    target_link_libraries(source_peer_host_takeover_tests PRIVATE halo2_authority_receiver nlohmann_json::nlohmann_json)
    add_test(NAME source_peer_host_takeover_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_peer_host_takeover_http_test.py" $<TARGET_FILE:source_peer_host_takeover_tests>)
    set_tests_properties(source_peer_host_takeover_http PROPERTIES TIMEOUT 270 SKIP_RETURN_CODE 77)
    add_executable(source_replay_batch_tests tests/source_replay_batch_tests.cpp)
    target_link_libraries(source_replay_batch_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_replay_batch COMMAND source_replay_batch_tests)
    add_executable(source_authority_replay_tests tests/source_authority_replay_tests.cpp)
    target_link_libraries(source_authority_replay_tests PRIVATE halo2_authority_receiver halo2_authority_publisher LibDataChannel::LibDataChannelStatic)
    add_test(NAME source_host_continuation_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
    set_tests_properties(source_host_continuation_http PROPERTIES TIMEOUT 330
        ENVIRONMENT "HALO2_HOST_CONTINUATION_CODEC=1;HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-host-continuation")
    add_test(NAME source_host_continuation_stream_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
    set_tests_properties(source_host_continuation_stream_http PROPERTIES TIMEOUT 330
        ENVIRONMENT "HALO2_HOST_CONTINUATION_STREAM=1;HALO2_RECEIVER_ASYNC=1;HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-host-stream")
    add_test(NAME source_authority_replay_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
    set_tests_properties(source_authority_replay_http PROPERTIES TIMEOUT 330
        ENVIRONMENT "HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-authority-replay")
    add_test(NAME source_authority_replay_cold_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
    set_tests_properties(source_authority_replay_cold_http PROPERTIES TIMEOUT 330
        ENVIRONMENT "HALO2_RECEIVER_COLD_FROM_CUT=1;HALO2_RECEIVER_REJECT_PREFETCH=1;HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-authority-replay-cold")
    add_test(NAME source_authority_replay_background_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
    set_tests_properties(source_authority_replay_background_http PROPERTIES TIMEOUT 330
        ENVIRONMENT "HALO2_RECEIVER_ASYNC=1;HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-authority-replay-background")
    add_test(NAME source_authority_replay_retention COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_replay_tests>)
    set_tests_properties(source_authority_replay_retention PROPERTIES TIMEOUT 330
        ENVIRONMENT "HALO2_RECEIVER_ASYNC=1;HALO2_RECEIVER_RETAIN_INFLIGHT=1;HALO2_RECEIVER_REJECT_PREFETCH=1;HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-authority-replay-retention")
    add_executable(source_execution_codec_tests tests/source_execution_codec_tests.cpp)
    target_link_libraries(source_execution_codec_tests PRIVATE halo2_authority_codec OpenSSL::Crypto ZLIB::ZLIB)
    add_test(NAME source_execution_codec COMMAND source_execution_codec_tests)
    add_executable(source_authority_receiver_tests tests/source_authority_receiver_tests.cpp)
    target_link_libraries(source_authority_receiver_tests PRIVATE halo2_authority_receiver halo2_authority_publisher LibDataChannel::LibDataChannelStatic)
    add_test(NAME source_authority_receiver_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_receiver_tests>)
    set_tests_properties(source_authority_receiver_http PROPERTIES TIMEOUT 330)
    add_test(NAME source_authority_receiver_cold_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_receiver_tests>)
    set_tests_properties(source_authority_receiver_cold_http PROPERTIES TIMEOUT 330
        ENVIRONMENT "HALO2_RECEIVER_COLD_FROM_CUT=1;HALO2_PUBLISHER_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-authority-receiver-cold")
    add_executable(source_authority_publisher_tests tests/source_authority_publisher_tests.cpp)
    target_link_libraries(source_authority_publisher_tests PRIVATE halo2_authority_publisher halo2_authority_delta LibDataChannel::LibDataChannelStatic)
    add_test(NAME source_authority_publisher_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_publisher_tests>)
    set_tests_properties(source_authority_publisher_http PROPERTIES TIMEOUT 330)
    add_executable(source_authority_delta_tests tests/source_authority_delta_tests.cpp)
    target_link_libraries(source_authority_delta_tests PRIVATE halo2_authority_delta)
    add_test(NAME source_authority_delta COMMAND source_authority_delta_tests)
    add_executable(source_authority_peer_tests tests/source_authority_peer_tests.cpp)
    target_link_libraries(source_authority_peer_tests PRIVATE halo2_authority_codec halo2_authority_delta halo2_authority_transfer halo2_online_peers)
    add_test(NAME source_authority_peer_tu15 COMMAND source_authority_peer_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_authority_peer_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_authority_codec_tests tests/source_authority_codec_tests.cpp)
    target_link_libraries(source_authority_codec_tests PRIVATE halo2_authority_codec)
    add_executable(source_authority_schema3_tests tests/source_authority_schema3_tests.cpp)
    target_link_libraries(source_authority_schema3_tests PRIVATE halo2_authority_codec)
    add_test(NAME source_authority_schema3 COMMAND source_authority_schema3_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_object_attachment_tu15.json")
    add_test(NAME source_authority_codec_tu15 COMMAND source_authority_codec_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_authority_codec_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
endif()
