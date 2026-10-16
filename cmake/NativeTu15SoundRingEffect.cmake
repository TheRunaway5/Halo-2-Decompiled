# Typed records8/9 and retained histories. Complete GP/EP/producer/45 remain required.
add_library(halo2_native_tu15_sound_ring_effect STATIC src/native_tu15_sound_ring_effect.cpp)
target_include_directories(halo2_native_tu15_sound_ring_effect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_ring_effect PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_ring_effect)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_ring_effect_retained_probe tests/native_tu15_sound_ring_effect_retained_probe.cpp)
    target_link_libraries(native_tu15_sound_ring_effect_retained_probe PRIVATE halo2_native_tu15_sound_ring_effect)
    add_executable(native_tu15_sound_ring_effect_guards_tests tests/native_tu15_sound_ring_effect_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_ring_effect_guards_tests PRIVATE halo2_native_tu15_sound_ring_effect)
    add_test(NAME native_tu15_sound_ring_effect_reference_vectors
        COMMAND "${Python3_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_ring_effect.py
        --probe $<TARGET_FILE:native_tu15_sound_ring_effect_retained_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_ring_effect_vectors.json.gz
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-ring-effect-report.json)
    set_tests_properties(native_tu15_sound_ring_effect_reference_vectors PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_sound_ring_effect_admission_guards COMMAND native_tu15_sound_ring_effect_guards_tests)
    set_tests_properties(native_tu15_sound_ring_effect_admission_guards PROPERTIES TIMEOUT 60)
endif()
