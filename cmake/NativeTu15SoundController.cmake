# GraphStorage owns four Sink objects; controller wrappers access GraphStorage.
# Keep both implementations in one archive without a static dependency cycle.
target_sources(halo2_native_tu15_sound_graph_storage PRIVATE
    src/audio/native_tu15_sound_controller.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_controller_probe
        tests/native_tu15_sound_controller_probe.cpp)
    target_link_libraries(native_tu15_sound_controller_probe PRIVATE
        halo2_native_tu15_sound_graph_storage nlohmann_json::nlohmann_json)
    add_executable(native_tu15_sound_controller_guards_tests
        tests/native_tu15_sound_controller_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_controller_guards_tests PRIVATE
        halo2_native_tu15_sound_graph_storage)
    add_test(NAME native_tu15_sound_controller_retained_reference_vectors
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_controller.py
        --probe $<TARGET_FILE:native_tu15_sound_controller_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_controller_vectors.json.gz
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-controller-report.json)
    add_test(NAME native_tu15_sound_controller_admission_guards
        COMMAND native_tu15_sound_controller_guards_tests)
    set_tests_properties(native_tu15_sound_controller_retained_reference_vectors
        native_tu15_sound_controller_admission_guards PROPERTIES TIMEOUT 90)
endif()
