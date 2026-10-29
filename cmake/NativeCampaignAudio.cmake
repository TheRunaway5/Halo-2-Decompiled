include(${CMAKE_CURRENT_LIST_DIR}/NativeTu15DataArrayDelete.cmake)
target_link_libraries(halo2_physics_world_runtime PRIVATE halo2_native_tu15_data_array_delete)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_campaign_audio_leaves_tu15_tests
        tests/native_campaign_audio_leaves_tu15_tests.cpp
        src/audio/native_tu15_sound_kernels_campaign_channels.cpp)
    target_include_directories(native_campaign_audio_leaves_tu15_tests PRIVATE include src)
    target_link_libraries(native_campaign_audio_leaves_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(native_campaign_audio_leaves_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    add_test(NAME native_campaign_audio_leaves_tu15
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_native_campaign_audio_leaves_tu15.py
            --native $<TARGET_FILE:native_campaign_audio_leaves_tu15_tests>
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-campaign-audio-leaves-tu15")
    set_tests_properties(native_campaign_audio_leaves_tu15 PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    add_executable(native_campaign_audio_ducking_tu15_tests
        tests/native_campaign_audio_ducking_tu15_tests.cpp)
    target_include_directories(native_campaign_audio_ducking_tu15_tests PRIVATE include src)
    target_link_libraries(native_campaign_audio_ducking_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(native_campaign_audio_ducking_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    add_test(NAME native_campaign_audio_ducking_tu15
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_native_campaign_ducking_tu15.py
            --native $<TARGET_FILE:native_campaign_audio_ducking_tu15_tests>
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-campaign-audio-ducking-tu15")
    set_tests_properties(native_campaign_audio_ducking_tu15 PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    add_executable(native_campaign_audio_fades_tu15_tests
        tests/native_campaign_audio_fades_tu15_tests.cpp)
    target_include_directories(native_campaign_audio_fades_tu15_tests PRIVATE include src)
    target_link_libraries(native_campaign_audio_fades_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(native_campaign_audio_fades_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    add_test(NAME native_campaign_audio_fades_tu15
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_native_campaign_fades_tu15.py
            --native $<TARGET_FILE:native_campaign_audio_fades_tu15_tests>
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-campaign-audio-fades-tu15")
    set_tests_properties(native_campaign_audio_fades_tu15 PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    add_executable(native_campaign_audio_listeners_tu15_tests
        tests/native_campaign_audio_listeners_tu15_tests.cpp
        src/audio/native_tu15_sound_kernels_listener.cpp)
    target_include_directories(native_campaign_audio_listeners_tu15_tests PRIVATE include src)
    target_link_libraries(native_campaign_audio_listeners_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(native_campaign_audio_listeners_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    add_test(NAME native_campaign_audio_listeners_tu15
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_native_campaign_listeners_tu15.py
            --native $<TARGET_FILE:native_campaign_audio_listeners_tu15_tests>
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-campaign-audio-listeners-tu15")
    set_tests_properties(native_campaign_audio_listeners_tu15 PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    add_executable(native_campaign_audio_retirement_tu15_tests
        tests/native_campaign_audio_retirement_tu15_tests.cpp)
    target_include_directories(native_campaign_audio_retirement_tu15_tests PRIVATE include src)
    target_link_libraries(native_campaign_audio_retirement_tu15_tests PRIVATE
        nlohmann_json::nlohmann_json halo2_native_tu15_data_array_delete)
    add_test(NAME native_campaign_audio_retirement_tu15
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_native_campaign_retirement_tu15.py
            --native $<TARGET_FILE:native_campaign_audio_retirement_tu15_tests>
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-campaign-audio-retirement-tu15")
    set_tests_properties(native_campaign_audio_retirement_tu15 PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    add_executable(native_campaign_audio_stop_callback_tu15_tests
        tests/native_campaign_audio_stop_callback_tu15_tests.cpp
        tests/native_campaign_audio_stop_manager_groups.cpp src/audio/native_tu15_sound_manager.cpp)
    target_include_directories(native_campaign_audio_stop_callback_tu15_tests PRIVATE include src)
    target_link_libraries(native_campaign_audio_stop_callback_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME native_campaign_audio_stop_callback_tu15
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_native_campaign_stop_callback_tu15.py
            --native $<TARGET_FILE:native_campaign_audio_stop_callback_tu15_tests>
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-campaign-audio-stop-callback-tu15")
    set_tests_properties(native_campaign_audio_stop_callback_tu15 PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    add_executable(native_campaign_listener_contract_tests
        tests/native_campaign_listener_contract_tests.cpp
        src/audio/native_tu15_sound_kernels_listener.cpp src/audio/native_tu15_sound_manager.cpp)
    target_include_directories(native_campaign_listener_contract_tests PRIVATE include src)
    target_compile_options(native_campaign_listener_contract_tests PRIVATE
        -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_options(native_campaign_listener_contract_tests PRIVATE -Wl,--gc-sections)
    add_test(NAME native_campaign_listener_contract COMMAND native_campaign_listener_contract_tests)
    add_executable(native_campaign_audio_gate_tests tests/native_campaign_audio_gate_tests.cpp
        src/audio/native_tu15_sound_kernels_campaign_channels.cpp)
    target_include_directories(native_campaign_audio_gate_tests PRIVATE include src)
    target_link_libraries(native_campaign_audio_gate_tests PRIVATE halo2_native_tu15_data_array_delete)
    add_test(NAME native_campaign_audio_semantic_gate COMMAND native_campaign_audio_gate_tests)
endif()
