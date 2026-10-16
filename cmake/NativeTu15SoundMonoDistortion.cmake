# Compiled original record-1 arithmetic only. Other effects, source producer,
# voice/DMA clocks and complete cold initializer 45 remain required.
add_library(halo2_native_tu15_sound_mono_distortion STATIC
    src/native_tu15_sound_mono_distortion.cpp)
target_include_directories(halo2_native_tu15_sound_mono_distortion PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_mono_distortion PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_mono_distortion)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_mono_distortion_probe
        tests/native_tu15_sound_mono_distortion_probe.cpp)
    target_link_libraries(native_tu15_sound_mono_distortion_probe PRIVATE
        halo2_native_tu15_sound_mono_distortion)
    add_executable(native_tu15_sound_mono_distortion_guards_tests
        tests/native_tu15_sound_mono_distortion_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_mono_distortion_guards_tests PRIVATE
        halo2_native_tu15_sound_mono_distortion)
    add_test(NAME native_tu15_sound_mono_distortion_reference_vectors
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_mono_distortion.py
        --probe $<TARGET_FILE:native_tu15_sound_mono_distortion_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_mono_distortion_vectors.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-mono-distortion-report.json)
    set_tests_properties(native_tu15_sound_mono_distortion_reference_vectors PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_mono_distortion_admission_guards
        COMMAND native_tu15_sound_mono_distortion_guards_tests)
    set_tests_properties(native_tu15_sound_mono_distortion_admission_guards PROPERTIES TIMEOUT 60)
endif()
