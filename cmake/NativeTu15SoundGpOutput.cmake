# Retained logical output values and completed native-copy receipts only.
add_library(halo2_native_tu15_sound_gp_output STATIC
    src/audio/dsp/native_tu15_sound_gp_output.cpp)
target_include_directories(halo2_native_tu15_sound_gp_output PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_gp_output PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_gp_output)
if(BUILD_TESTING)
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
