# Bounded no-scaling record0 status projection; boot/whole-graph ownership stays explicit.
add_library(halo2_native_tu15_sound_record0_context STATIC
    src/audio/native_tu15_sound_status_context.cpp
    src/audio/native_tu15_sound_record0_context.cpp)
target_include_directories(halo2_native_tu15_sound_record0_context PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_record0_context PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_record0_context)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_record0_context_probe
        tests/native_tu15_sound_record0_context_probe.cpp)
    target_link_libraries(native_tu15_sound_record0_context_probe PRIVATE
        halo2_native_tu15_sound_record0_context
        halo2_native_tu15_sound_gain_fanout nlohmann_json::nlohmann_json)
    add_executable(native_tu15_sound_record0_context_guards_tests
        tests/native_tu15_sound_record0_context_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_record0_context_guards_tests PRIVATE
        halo2_native_tu15_sound_record0_context)
    add_executable(native_tu15_sound_record0_context_provenance_tests
        tests/native_tu15_sound_record0_context_provenance_tests.cpp)
    target_link_libraries(native_tu15_sound_record0_context_provenance_tests PRIVATE
        halo2_native_tu15_sound_record0_context)
    add_test(NAME native_tu15_sound_record0_context_retained_reference
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_record0_context.py
        --probe $<TARGET_FILE:native_tu15_sound_record0_context_probe>
        --cases ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_record0_context_cases.json.gz
        --proof ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_record0_context_proof.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-record0-context-report.json)
    add_test(NAME native_tu15_sound_record0_context_admission_guards
        COMMAND native_tu15_sound_record0_context_guards_tests)
    add_test(NAME native_tu15_sound_record0_context_provenance_boundaries
        COMMAND native_tu15_sound_record0_context_provenance_tests)
    set_tests_properties(native_tu15_sound_record0_context_retained_reference
        native_tu15_sound_record0_context_admission_guards
        native_tu15_sound_record0_context_provenance_boundaries PROPERTIES TIMEOUT 120)
endif()
