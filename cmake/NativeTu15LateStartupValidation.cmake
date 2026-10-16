# Optional independent component validation. This never starts a cold chain
# past slot45 and adds no production startup bypass or readiness declaration.
if(BUILD_TESTING)
    add_executable(native_tu15_late_startup_storage_tests
        tests/native_tu15_late_startup_storage_tests.cpp
        src/source_startup_sound_storage_tu15_generated.cpp
        src/native_tu15_startup_sound_storage_data.cpp
        src/source_render_object_startup_tu15_generated.cpp
        src/source_ui_script_startup_tu15_generated.cpp
        src/native_tu15_ui_script_startup_data.cpp
        src/native_tu15_storage_formatting.cpp
        src/source_final_startup_storage_tu15_generated.cpp
        src/native_tu15_final_startup_storage_data.cpp
        src/source_runtime_memory.cpp src/source_runtime_host_storage.cpp
        src/physics_pool_host_storage.cpp)
    target_include_directories(native_tu15_late_startup_storage_tests PRIVATE src)
    target_compile_features(native_tu15_late_startup_storage_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_late_startup_storage_tests PRIVATE
        halo2_physics_world_runtime halo2_raw_datum_pool_identity
        halo2_snapshot_storage nlohmann_json::nlohmann_json Threads::Threads)
    target_compile_options(native_tu15_late_startup_storage_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_late_startup_storage_component
        COMMAND native_tu15_late_startup_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-late-startup-storage/original-v2/report.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-late-startup-storage-differential.json)
    set_tests_properties(original_tu15_late_startup_storage_component PROPERTIES TIMEOUT 60)

    add_executable(native_tu15_late_startup_simd_status_tests
        tests/native_tu15_late_startup_simd_status_tests.cpp)
    target_link_libraries(native_tu15_late_startup_simd_status_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_features(native_tu15_late_startup_simd_status_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_late_startup_simd_status_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_late_startup_simd_status_calibration
        COMMAND native_tu15_late_startup_simd_status_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-late-startup-storage/original-v2/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-late-startup-simd-status-calibration.json)
    set_tests_properties(original_tu15_late_startup_simd_status_calibration PROPERTIES TIMEOUT 60)
endif()
