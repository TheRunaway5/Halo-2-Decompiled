# Compiled original record-0 ordered arithmetic. The complete DSP graph,
# source producer and cold initializer45 remain required.
add_library(halo2_native_tu15_sound_gain_fanout STATIC
    src/audio/native_tu15_sound_gain_fanout.cpp)
target_include_directories(halo2_native_tu15_sound_gain_fanout PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_gain_fanout PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_gain_fanout)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_gain_fanout_probe
        tests/native_tu15_sound_gain_fanout_probe.cpp)
    target_link_libraries(native_tu15_sound_gain_fanout_probe PRIVATE
        halo2_native_tu15_sound_gain_fanout)
    add_executable(native_tu15_sound_gain_fanout_guards_tests
        tests/native_tu15_sound_gain_fanout_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_gain_fanout_guards_tests PRIVATE
        halo2_native_tu15_sound_gain_fanout)
    add_test(NAME native_tu15_sound_gain_fanout_reference_vectors
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_gain_fanout.py
        --probe $<TARGET_FILE:native_tu15_sound_gain_fanout_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_gain_fanout_vectors.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-gain-fanout-report.json)
    set_tests_properties(native_tu15_sound_gain_fanout_reference_vectors PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_gain_fanout_admission_guards
        COMMAND native_tu15_sound_gain_fanout_guards_tests)
    set_tests_properties(native_tu15_sound_gain_fanout_admission_guards PROPERTIES TIMEOUT 60)
endif()
