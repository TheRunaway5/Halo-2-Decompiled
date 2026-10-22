add_library(halo2_native_tu15_sound_graph_storage STATIC src/audio/native_tu15_sound_graph_storage.cpp)
target_include_directories(halo2_native_tu15_sound_graph_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_graph_storage PUBLIC cxx_std_20)
target_link_libraries(halo2_native_tu15_sound_graph_storage PUBLIC
    halo2_native_tu15_sound_gain_fanout
    halo2_native_tu15_sound_mono_distortion
    halo2_native_tu15_sound_two_product_accumulator
    halo2_native_tu15_sound_chorus
    halo2_native_tu15_sound_crosstalk
    halo2_native_tu15_sound_controller_arithmetic
    halo2_native_tu15_sound_ring_effect)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_graph_storage)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_graph_storage_probe tests/native_tu15_sound_graph_storage_probe.cpp)
    target_link_libraries(native_tu15_sound_graph_storage_probe PRIVATE halo2_native_tu15_sound_graph_storage)
    add_executable(native_tu15_sound_graph_storage_guards_tests tests/native_tu15_sound_graph_storage_guards_tests.cpp)
    target_link_libraries(native_tu15_sound_graph_storage_guards_tests PRIVATE halo2_native_tu15_sound_graph_storage)
    add_test(NAME native_tu15_sound_graph_storage_literal_fields
        COMMAND "${Python3_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_graph_storage.py
        --probe $<TARGET_FILE:native_tu15_sound_graph_storage_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_graph_storage_vectors.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-graph-storage-report.json)
    add_test(NAME native_tu15_sound_graph_storage_admission_guards COMMAND native_tu15_sound_graph_storage_guards_tests)
    set_tests_properties(native_tu15_sound_graph_storage_literal_fields native_tu15_sound_graph_storage_admission_guards PROPERTIES TIMEOUT 60)
endif()
