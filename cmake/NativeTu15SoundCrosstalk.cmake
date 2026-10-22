# Compiled typed original crosstalk arithmetic; whole DSP/producer/45 remain required.
add_library(halo2_native_tu15_sound_crosstalk STATIC src/audio/dsp/native_tu15_sound_crosstalk.cpp)
target_include_directories(halo2_native_tu15_sound_crosstalk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_crosstalk PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_crosstalk)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_crosstalk_probe tests/native_tu15_sound_crosstalk_probe.cpp)
    target_link_libraries(native_tu15_sound_crosstalk_probe PRIVATE halo2_native_tu15_sound_crosstalk)
    add_executable(native_tu15_sound_crosstalk_guards_tests tests/native_tu15_sound_crosstalk_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_crosstalk_guards_tests PRIVATE halo2_native_tu15_sound_crosstalk)
    add_test(NAME native_tu15_sound_crosstalk_reference_vectors
        COMMAND "${Python3_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_crosstalk.py
        --probe $<TARGET_FILE:native_tu15_sound_crosstalk_probe>
        --join-vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_crosstalk_join_vectors.json
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_crosstalk_vectors.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-crosstalk-report.json)
    set_tests_properties(native_tu15_sound_crosstalk_reference_vectors PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_crosstalk_admission_guards COMMAND native_tu15_sound_crosstalk_guards_tests)
    set_tests_properties(native_tu15_sound_crosstalk_admission_guards PROPERTIES TIMEOUT 60)
endif()
