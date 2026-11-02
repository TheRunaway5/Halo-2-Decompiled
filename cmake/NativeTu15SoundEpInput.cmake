# Actual retained GP logical values -> owned original EP input windows.
# The caller still owns frame phase/time; no physical DMA or endpoint ACK.
add_library(halo2_native_tu15_sound_ep_input STATIC src/audio/dsp/native_tu15_sound_ep_input.cpp)
target_include_directories(halo2_native_tu15_sound_ep_input PUBLIC include)
target_compile_features(halo2_native_tu15_sound_ep_input PUBLIC cxx_std_20)
target_link_libraries(halo2_native_tu15_sound_ep_input PUBLIC halo2_native_tu15_sound_gp_output)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_ep_input)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_ep_input_tests tests/native_tu15_sound_ep_input_tests.cpp)
    target_link_libraries(native_tu15_sound_ep_input_tests PRIVATE halo2_native_tu15_sound_ep_input
        halo2_native_tu15_sound_ep_stereo_numeric nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_ep_input_reference COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_ep_input.py
        --probe $<TARGET_FILE:native_tu15_sound_ep_input_tests>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_gp_output_values.bin.gz
        --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_ep_input_join_tu15.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-ep-input-report.json)
    set_tests_properties(native_tu15_sound_ep_input_reference PROPERTIES TIMEOUT 90)
endif()
