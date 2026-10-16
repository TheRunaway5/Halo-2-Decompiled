# Compiled typed original chorus arithmetic; whole DSP/producer/45 remain required.
add_library(halo2_native_tu15_sound_chorus STATIC src/native_tu15_sound_chorus.cpp)
target_include_directories(halo2_native_tu15_sound_chorus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_chorus PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_chorus)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_chorus_probe tests/native_tu15_sound_chorus_probe.cpp)
    target_link_libraries(native_tu15_sound_chorus_probe PRIVATE halo2_native_tu15_sound_chorus)
    add_executable(native_tu15_sound_chorus_guards_tests tests/native_tu15_sound_chorus_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_chorus_guards_tests PRIVATE halo2_native_tu15_sound_chorus)
    add_executable(native_tu15_sound_chorus_coupled_probe tests/native_tu15_sound_chorus_coupled_probe.cpp)
    target_link_libraries(native_tu15_sound_chorus_coupled_probe PRIVATE halo2_native_tu15_sound_chorus)
    add_test(NAME native_tu15_sound_chorus_reference_vectors
        COMMAND "${Python3_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_chorus.py
        --probe $<TARGET_FILE:native_tu15_sound_chorus_probe>
        --coupled-probe $<TARGET_FILE:native_tu15_sound_chorus_coupled_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_chorus_vectors.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-chorus-report.json)
    set_tests_properties(native_tu15_sound_chorus_reference_vectors PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_chorus_admission_guards COMMAND native_tu15_sound_chorus_guards_tests)
    set_tests_properties(native_tu15_sound_chorus_admission_guards PROPERTIES TIMEOUT 60)
endif()
