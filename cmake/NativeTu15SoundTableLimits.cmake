# Actual retained table capacities only; no full38E8C9 policy/readiness.
target_sources(halo2_systems PRIVATE src/audio/runtime/source_runtime_sound_table_limits.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_table_limits_tests
        tests/native_tu15_sound_table_limits_tests.cpp src/audio/runtime/source_runtime_sound_table_limits.cpp
        src/audio/runtime/source_runtime_sound_ep_output_resources.cpp
        src/audio/native_tu15_audio_preference.cpp src/audio/native_tu15_sound_sdk_prefix_data.cpp
        src/audio/runtime/source_runtime_sound_sdk_resources.cpp src/audio/source_sound_sdk_pool.cpp
        src/audio/runtime/source_runtime_sound_dma_resources.cpp src/runtime/source_runtime_host_physical_memory.cpp
        src/audio/runtime/source_runtime_sound_reset_resources.cpp src/audio/source_sound_reset_tu15_generated.cpp
        src/audio/source_sound_hardware_storage_tu15_generated.cpp src/audio/source_sound_dma_storage_tu15_generated.cpp
        src/audio/source_sound_sdk_tu15_generated.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_table_limits_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_table_limits_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage halo2_source_tu15_settings
        nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_table_limits_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_native_sound_table_limits
        COMMAND native_tu15_sound_table_limits_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-configuration/original-v3/report.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-table-bindings/original-fragment/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-table-limits-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-table-limits-report.json)
    set_tests_properties(original_tu15_native_sound_table_limits PROPERTIES TIMEOUT 60)
endif()
