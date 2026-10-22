# Typed controller records11..14 in-place arithmetic only. Source DMA/scratch,
# producer/clock/PCM, full DSP graph and initializer45 remain required.
add_library(halo2_native_tu15_sound_controller_arithmetic STATIC
    src/audio/dsp/native_tu15_sound_controller_arithmetic.cpp)
target_include_directories(halo2_native_tu15_sound_controller_arithmetic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_controller_arithmetic PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_controller_arithmetic)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_controller_arithmetic_probe
        tests/native_tu15_sound_controller_arithmetic_probe.cpp)
    target_link_libraries(native_tu15_sound_controller_arithmetic_probe PRIVATE
        halo2_native_tu15_sound_controller_arithmetic)
    add_executable(native_tu15_sound_controller_arithmetic_guards_tests
        tests/native_tu15_sound_controller_arithmetic_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_controller_arithmetic_guards_tests PRIVATE
        halo2_native_tu15_sound_controller_arithmetic)
    add_test(NAME native_tu15_sound_controller_arithmetic_reference_vectors
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_controller_arithmetic.py
        --probe $<TARGET_FILE:native_tu15_sound_controller_arithmetic_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_controller_arithmetic_vectors.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-controller-arithmetic-report.json)
    set_tests_properties(native_tu15_sound_controller_arithmetic_reference_vectors PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_controller_arithmetic_admission_guards
        COMMAND native_tu15_sound_controller_arithmetic_guards_tests)
    set_tests_properties(native_tu15_sound_controller_arithmetic_admission_guards PROPERTIES TIMEOUT 60)
endif()
