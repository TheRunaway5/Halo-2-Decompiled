# Retained logical output values and completed native-copy receipts only.
add_library(halo2_native_tu15_sound_gp_output STATIC
    src/audio/dsp/native_tu15_sound_gp_output.cpp
    src/audio/dsp/native_tu15_sound_wet_return_sources.cpp
    src/audio/source_sound_wet_returns_tu15.cpp
    src/audio/source_sound_wet_voice_layout_tu15.cpp)
target_include_directories(halo2_native_tu15_sound_gp_output PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_gp_output PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_gp_output)
target_link_libraries(halo2_cache PUBLIC halo2_native_tu15_sound_gp_output)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_wet_voice_layout_tests tests/native_tu15_sound_wet_voice_layout_tests.cpp
        src/audio/sound_sdk_voice_attachment.cpp src/audio/sound_sdk_voice_initialization.cpp
        src/audio/sound_sdk_voice_dispatch.cpp src/audio/sound_sdk_hardware_voice.cpp
        src/audio/sound_sdk_device.cpp src/audio/sound_sdk_spatial.cpp src/audio/sound_sdk_sample_pages.cpp)
    target_link_libraries(native_tu15_sound_wet_voice_layout_tests PRIVATE halo2_native_tu15_sound_gp_output nlohmann_json::nlohmann_json Threads::Threads)
    add_test(NAME native_tu15_sound_wet_voice_layout_reference COMMAND native_tu15_sound_wet_voice_layout_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_wet_voice_layout_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/combined_wet_graph_tu15.json")
    set_tests_properties(native_tu15_sound_wet_voice_layout_reference PROPERTIES TIMEOUT 60)
    add_executable(native_tu15_sound_wet_returns_tests tests/native_tu15_sound_wet_returns_tests.cpp
        src/audio/sound_driver_startup.cpp)
    target_link_libraries(native_tu15_sound_wet_returns_tests PRIVATE
        halo2_native_tu15_sound_gp_output halo2_native_tu15_sound_graph_storage nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_wet_returns_reference COMMAND native_tu15_sound_wet_returns_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_wet_returns_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/combined_wet_graph_tu15.json")
    set_tests_properties(native_tu15_sound_wet_returns_reference PROPERTIES TIMEOUT 60)
    add_executable(native_tu15_sound_gp_output_probe
        tests/native_tu15_sound_gp_output_probe.cpp)
    target_link_libraries(native_tu15_sound_gp_output_probe PRIVATE
        halo2_native_tu15_sound_gp_output)
    add_test(NAME native_tu15_sound_gp_output_retained_values_and_ownership
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_gp_output.py
        --probe $<TARGET_FILE:native_tu15_sound_gp_output_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_gp_output_values.bin.gz
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-gp-output-report.json)
    set_tests_properties(native_tu15_sound_gp_output_retained_values_and_ownership
        PROPERTIES TIMEOUT 90)
endif()
