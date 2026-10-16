# Real bounded payload queue/notifier/input page prerequisite. No cold
# configuration bypass, tracking/voice/DSP implementation or source return.
target_sources(halo2_systems PRIVATE src/source_runtime_sound_pio_resources.cpp
    src/source_sound_pio_storage_tu15_generated.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_pio_resources_tests
        tests/native_tu15_sound_pio_resources_tests.cpp
        src/source_runtime_sound_pio_resources.cpp src/source_sound_pio_storage_tu15_generated.cpp
        src/source_runtime_sound_table_limits.cpp src/source_runtime_sound_ep_output_resources.cpp
        src/native_tu15_audio_preference.cpp src/native_tu15_sound_sdk_prefix_data.cpp
        src/source_runtime_sound_sdk_resources.cpp src/source_sound_sdk_pool.cpp
        src/source_runtime_sound_dma_resources.cpp src/source_runtime_host_physical_memory.cpp
        src/source_runtime_sound_reset_resources.cpp src/source_sound_reset_tu15_generated.cpp
        src/source_sound_hardware_storage_tu15_generated.cpp src/source_sound_dma_storage_tu15_generated.cpp
        src/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_pio_resources_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_pio_resources_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage halo2_source_tu15_settings
        nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_pio_resources_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_native_sound_pio_resources
        COMMAND native_tu15_sound_pio_resources_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-frontend-executor-contract/original/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-pio-resources-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-pio-resources-report.json)
    set_tests_properties(original_tu15_native_sound_pio_resources PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_pio_foreign_destructor
        COMMAND "${Python3_EXECUTABLE}" -c
        "import subprocess, sys; result = subprocess.run(sys.argv[1:]); sys.exit(0 if result.returncode == 77 else 1)"
        $<TARGET_FILE:native_tu15_sound_pio_resources_tests> --foreign-destructor-probe
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-frontend-executor-contract/original/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-pio-foreign-destructor-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-pio-foreign-destructor-unused-report.json)
    set_tests_properties(native_tu15_sound_pio_foreign_destructor PROPERTIES TIMEOUT 60)
endif()
