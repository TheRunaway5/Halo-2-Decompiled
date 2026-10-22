# Compiled original records2/3 with explicit entry status and per-operation
# saturation. Complete source DSP/voice producer and initializer45 remain required.
add_library(halo2_native_tu15_sound_two_product_accumulator STATIC
    src/audio/dsp/native_tu15_sound_two_product_accumulator.cpp)
target_include_directories(halo2_native_tu15_sound_two_product_accumulator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_two_product_accumulator PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_two_product_accumulator)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_two_product_accumulator_probe
        tests/native_tu15_sound_two_product_accumulator_probe.cpp)
    target_link_libraries(native_tu15_sound_two_product_accumulator_probe PRIVATE
        halo2_native_tu15_sound_two_product_accumulator)
    add_executable(native_tu15_sound_two_product_accumulator_guards_tests
        tests/native_tu15_sound_two_product_accumulator_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_two_product_accumulator_guards_tests PRIVATE
        halo2_native_tu15_sound_two_product_accumulator)
    add_test(NAME native_tu15_sound_two_product_accumulator_reference_vectors
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_two_product_accumulator.py
        --probe $<TARGET_FILE:native_tu15_sound_two_product_accumulator_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_two_product_accumulator_vectors.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-two-product-accumulator-report.json)
    set_tests_properties(native_tu15_sound_two_product_accumulator_reference_vectors PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_two_product_accumulator_admission_guards
        COMMAND native_tu15_sound_two_product_accumulator_guards_tests)
    set_tests_properties(native_tu15_sound_two_product_accumulator_admission_guards PROPERTIES TIMEOUT 60)
endif()
