find_package(ZLIB 1.2.9 REQUIRED)
add_library(halo2_authority_delta STATIC src/runtime/source_authority_delta.cpp)
target_include_directories(halo2_authority_delta PUBLIC include)
target_compile_features(halo2_authority_delta PUBLIC cxx_std_20)
target_link_libraries(halo2_authority_delta PRIVATE OpenSSL::Crypto ZLIB::ZLIB)
target_sources(halo2_systems PRIVATE src/runtime/source_authority_binding.cpp)
target_link_libraries(halo2_systems PRIVATE OpenSSL::Crypto)
add_library(halo2_authority_codec STATIC src/runtime/source_authority_codec.cpp src/runtime/source_authority_codec_decode.cpp src/runtime/source_execution_codec.cpp src/platform/source_replay_batch.cpp)
target_sources(halo2_authority_codec PRIVATE src/gameplay/vehicles/native_vehicle_camera_codec.cpp src/runtime/source_host_continuation_codec.cpp)
target_sources(halo2_authority_codec PRIVATE src/runtime/source_host_continuation_bundle.cpp)
target_compile_features(halo2_authority_codec PUBLIC cxx_std_20)
target_include_directories(halo2_authority_codec PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost-pfr/include")
target_link_libraries(halo2_authority_codec PUBLIC halo2_systems PRIVATE halo2_online_protocol OpenSSL::Crypto ZLIB::ZLIB)
add_library(halo2_authority_publisher STATIC src/runtime/source_authority_publisher.cpp)
target_compile_features(halo2_authority_publisher PUBLIC cxx_std_20)
target_include_directories(halo2_authority_publisher PUBLIC include)
target_link_libraries(halo2_authority_publisher PUBLIC halo2_authority_codec PRIVATE halo2_authority_delta Threads::Threads)
add_library(halo2_authority_receiver STATIC src/runtime/source_authority_receiver.cpp)
target_sources(halo2_authority_receiver PRIVATE src/rendering/effects/effect_presentation_history.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/multiplayer_pose_continuity.cpp)
target_sources(halo2_authority_receiver PRIVATE src/network/multiplayer_receipt_continuity.cpp)
target_sources(halo2_authority_receiver PRIVATE src/runtime/source_foreground_world_handoff.cpp)
target_sources(halo2_authority_receiver PRIVATE src/network/source_peer_host_takeover.cpp)
target_sources(halo2_authority_receiver PRIVATE src/network/multiplayer_runtime_authority.cpp)
target_sources(halo2_renderer PRIVATE src/rendering/renderer/material_presentation_continuity.cpp src/gameplay/multiplayer/multiplayer_scene_continuity.cpp src/ui/hud/multiplayer_hud_continuity.cpp src/gameplay/multiplayer/multiplayer_locomotion_continuity.cpp)
target_compile_features(halo2_authority_receiver PUBLIC cxx_std_20)
target_include_directories(halo2_authority_receiver PUBLIC include)
target_link_libraries(halo2_authority_receiver PUBLIC halo2_authority_codec PRIVATE halo2_authority_publisher halo2_authority_delta Threads::Threads)
target_link_libraries(halo2_renderer PUBLIC halo2_authority_receiver halo2_authority_publisher)
include(cmake/HudContinuation.cmake)
include(cmake/LocomotionContinuation.cmake)
include(cmake/MccOnlineUi.cmake)
if(BUILD_TESTING)
    include(cmake/MaterialContinuityTests.cmake)
    add_executable(source_peer_host_takeover_tests tests/source_peer_host_takeover_tests.cpp)
    target_link_libraries(source_peer_host_takeover_tests PRIVATE halo2_authority_receiver nlohmann_json::nlohmann_json)
    add_test(NAME source_peer_host_takeover_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_peer_host_takeover_http_test.py" $<TARGET_FILE:source_peer_host_takeover_tests>)
    set_tests_properties(source_peer_host_takeover_http PROPERTIES TIMEOUT 270 SKIP_RETURN_CODE 77
        LABELS "offline-Xbox-reference")
    set(HALO2_MCC_NATIVE_SCOPE_TEST_ROOT "" CACHE PATH
        "Explicit selected prepared MCC cache for genuine takeover native-scope validation")
    set(HALO2_MCC_NATIVE_SCOPE_TEST_MAP "" CACHE FILEPATH
        "Optional exact Lockout map within HALO2_MCC_NATIVE_SCOPE_TEST_ROOT")
    if(HALO2_MCC_NATIVE_SCOPE_TEST_ROOT)
        set(_halo2_native_scope_mcc_map_args)
        if(HALO2_MCC_NATIVE_SCOPE_TEST_MAP)
            list(APPEND _halo2_native_scope_mcc_map_args --map "${HALO2_MCC_NATIVE_SCOPE_TEST_MAP}")
        endif()
        add_test(NAME source_peer_host_takeover_mcc_http COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_peer_host_takeover_http_test.py"
            $<TARGET_FILE:source_peer_host_takeover_tests>
            --mcc-root "${HALO2_MCC_NATIVE_SCOPE_TEST_ROOT}" ${_halo2_native_scope_mcc_map_args})
        set_tests_properties(source_peer_host_takeover_mcc_http PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE
            LABELS "selected-MCC-native"
            ENVIRONMENT "SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
            AND NOT BUILD_SHARED_LIBS)
            # This separately selected renderer control never opens a window on
            # the user's display or selects a physical Vulkan device.
            find_program(HALO2_MCC_HELD_LENS_XVFB_RUN NAMES xvfb-run REQUIRED)
            find_file(HALO2_MCC_HELD_LENS_SOFTWARE_ICD NAMES lvp_icd.json
                PATHS /usr/share/vulkan/icd.d /etc/vulkan/icd.d REQUIRED)
            include(cmake/VulkanLensRetirementObserver.cmake)
            add_executable(source_peer_host_held_lens_tests tests/source_peer_host_takeover_tests.cpp)
            target_compile_definitions(source_peer_host_held_lens_tests PRIVATE HALO2_TAKEOVER_HELD_LENS_TEST=1)
            target_link_options(source_peer_host_held_lens_tests PRIVATE
                "LINKER:--wrap=_ZNK5halo219SourceRuntimeMemory4viewEjm")
            halo2_observe_vulkan_lens_retirement(source_peer_host_held_lens_tests)
            target_link_libraries(source_peer_host_held_lens_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
            add_test(NAME source_peer_host_held_lens_mcc_http
                COMMAND "${HALO2_MCC_HELD_LENS_XVFB_RUN}" -a -s "-screen 0 640x480x24"
                ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_peer_host_takeover_http_test.py"
                $<TARGET_FILE:source_peer_host_held_lens_tests>
                --mcc-root "${HALO2_MCC_NATIVE_SCOPE_TEST_ROOT}" ${_halo2_native_scope_mcc_map_args})
            set_tests_properties(source_peer_host_held_lens_mcc_http PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE
                LABELS "selected-MCC-native"
                ENVIRONMENT "HALO2_TAKEOVER_HELD_LENS_TEST=1;HALO2_TAKEOVER_HELD_LENS_PRIVATE_XVFB=1;SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=x11;VK_ICD_FILENAMES=${HALO2_MCC_HELD_LENS_SOFTWARE_ICD}")
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
                set_property(TEST source_peer_host_held_lens_mcc_http APPEND PROPERTY ENVIRONMENT_MODIFICATION
                    "VK_LAYER_PATH=set:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=path_list_prepend:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib")
            endif()
        endif()
        unset(_halo2_native_scope_mcc_map_args)
    elseif(HALO2_MCC_NATIVE_SCOPE_TEST_MAP)
        message(FATAL_ERROR "HALO2_MCC_NATIVE_SCOPE_TEST_MAP requires HALO2_MCC_NATIVE_SCOPE_TEST_ROOT")
    endif()
    add_executable(source_common_cut_runtime_tests tests/source_common_cut_runtime_tests.cpp)
    target_link_libraries(source_common_cut_runtime_tests PRIVATE halo2_authority_receiver nlohmann_json::nlohmann_json)
    add_test(NAME source_common_cut_runtime_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_common_cut_runtime_http_test.py" $<TARGET_FILE:source_common_cut_runtime_tests>
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_common_cut_runtime_http PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77
        LABELS "offline-Xbox-reference"
        ENVIRONMENT "SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
    set(HALO2_MCC_COMMON_CUT_TEST_ROOT "" CACHE PATH
        "Explicit selected prepared MCC cache for native common-cut receipt validation")
    set(HALO2_MCC_COMMON_CUT_TEST_MAP "" CACHE FILEPATH
        "Optional exact Lockout map within HALO2_MCC_COMMON_CUT_TEST_ROOT")
    if(HALO2_MCC_COMMON_CUT_TEST_ROOT)
        set(_halo2_common_cut_mcc_map_args)
        if(HALO2_MCC_COMMON_CUT_TEST_MAP)
            list(APPEND _halo2_common_cut_mcc_map_args --map "${HALO2_MCC_COMMON_CUT_TEST_MAP}")
        endif()
        add_test(NAME source_common_cut_runtime_mcc_http COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_common_cut_runtime_http_test.py"
            $<TARGET_FILE:source_common_cut_runtime_tests>
            --mcc-root "${HALO2_MCC_COMMON_CUT_TEST_ROOT}" ${_halo2_common_cut_mcc_map_args})
        set_tests_properties(source_common_cut_runtime_mcc_http PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
            LABELS "selected-MCC-native"
            ENVIRONMENT "SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        add_test(NAME source_common_cut_disconnect_mcc_http COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_common_cut_runtime_http_test.py"
            $<TARGET_FILE:source_common_cut_runtime_tests>
            --mcc-root "${HALO2_MCC_COMMON_CUT_TEST_ROOT}" ${_halo2_common_cut_mcc_map_args})
        set_tests_properties(source_common_cut_disconnect_mcc_http PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
            LABELS "selected-MCC-native"
            ENVIRONMENT "HALO2_COMMON_DISCONNECT=1;SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        add_test(NAME source_common_cut_prospective_mcc_http COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_common_cut_runtime_http_test.py"
            $<TARGET_FILE:source_common_cut_runtime_tests>
            --mcc-root "${HALO2_MCC_COMMON_CUT_TEST_ROOT}" ${_halo2_common_cut_mcc_map_args})
        set_tests_properties(source_common_cut_prospective_mcc_http PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
            LABELS "selected-MCC-native"
            ENVIRONMENT "HALO2_COMMON_PROSPECTIVE=1;SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        foreach(_halo2_inventory_case IN ITEMS abort freeze)
            add_test(NAME source_common_cut_inventory_${_halo2_inventory_case}_mcc_http COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_common_cut_runtime_http_test.py"
                $<TARGET_FILE:source_common_cut_runtime_tests>
                --mcc-root "${HALO2_MCC_COMMON_CUT_TEST_ROOT}" ${_halo2_common_cut_mcc_map_args})
            set_tests_properties(source_common_cut_inventory_${_halo2_inventory_case}_mcc_http PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
                LABELS "selected-MCC-native"
                ENVIRONMENT "HALO2_COMMON_INVENTORY=${_halo2_inventory_case};SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        endforeach()
        unset(_halo2_inventory_case)
        add_test(NAME source_common_cut_prospective_client_mcc_http COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_common_cut_runtime_http_test.py"
            $<TARGET_FILE:source_common_cut_runtime_tests>
            --mcc-root "${HALO2_MCC_COMMON_CUT_TEST_ROOT}" ${_halo2_common_cut_mcc_map_args})
        set_tests_properties(source_common_cut_prospective_client_mcc_http PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
            LABELS "selected-MCC-native"
            ENVIRONMENT "HALO2_COMMON_CLIENT=1;HALO2_COMMON_PROSPECTIVE=1;SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        add_test(NAME source_common_cut_prospective_client_abort_mcc_http COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_common_cut_runtime_http_test.py"
            $<TARGET_FILE:source_common_cut_runtime_tests>
            --mcc-root "${HALO2_MCC_COMMON_CUT_TEST_ROOT}" ${_halo2_common_cut_mcc_map_args})
        set_tests_properties(source_common_cut_prospective_client_abort_mcc_http PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
            LABELS "selected-MCC-native"
            ENVIRONMENT "HALO2_COMMON_INVENTORY=abort;HALO2_COMMON_CLIENT_ABORT=1;SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        add_test(NAME source_common_cut_preparation_mcc_http COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_common_cut_runtime_http_test.py"
            $<TARGET_FILE:source_common_cut_runtime_tests>
            --mcc-root "${HALO2_MCC_COMMON_CUT_TEST_ROOT}" ${_halo2_common_cut_mcc_map_args})
        set_tests_properties(source_common_cut_preparation_mcc_http PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
            LABELS "selected-MCC-native"
            ENVIRONMENT "HALO2_COMMON_INVENTORY=preparation;SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        unset(_halo2_common_cut_mcc_map_args)
    elseif(HALO2_MCC_COMMON_CUT_TEST_MAP)
        message(FATAL_ERROR "HALO2_MCC_COMMON_CUT_TEST_MAP requires HALO2_MCC_COMMON_CUT_TEST_ROOT")
    endif()
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
    add_dependencies(source_authority_publisher_tests source_authority_replay_tests multiplayer_tests)
    add_test(NAME source_authority_publisher_http COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_publisher_http_test.py" $<TARGET_FILE:source_authority_publisher_tests>)
    set_tests_properties(source_authority_publisher_http PROPERTIES TIMEOUT 330)
    # Denied replay capabilities must preserve live host work and still permit
    # a complete, independently prepared checkpoint publication.
    add_test(NAME source_authority_publisher COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_authority_security_http_test.py"
        $<TARGET_FILE:source_authority_publisher_tests> $<TARGET_FILE:source_authority_replay_tests>)
    set_tests_properties(source_authority_publisher PROPERTIES TIMEOUT 660)
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
