# Functional native codec ABI only; source constructor still requires a real
# IRQ6/notification/shutdown owner. No38E8C9 bypass or codec-ready publication.
target_sources(halo2_systems PRIVATE src/audio/runtime/source_runtime_sound_codec_route.cpp
    src/audio/source_sound_codec_initialization_tu15_generated.cpp
    src/audio/native_tu15_sound_codec_initialization_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_codec_initialization_tests
        tests/native_tu15_sound_codec_initialization_tests.cpp
        src/audio/runtime/source_runtime_sound_codec_route.cpp src/audio/source_sound_codec_initialization_tu15_generated.cpp
        src/audio/native_tu15_sound_codec_initialization_data.cpp
        src/audio/native_tu15_audio_preference.cpp src/audio/native_tu15_sound_sdk_prefix_data.cpp
        src/audio/runtime/source_runtime_sound_sdk_resources.cpp src/audio/source_sound_sdk_pool.cpp
        src/audio/runtime/source_runtime_sound_dma_resources.cpp src/runtime/source_runtime_host_physical_memory.cpp
        src/audio/runtime/source_runtime_sound_reset_resources.cpp src/audio/source_sound_reset_tu15_generated.cpp
        src/audio/source_sound_hardware_storage_tu15_generated.cpp src/audio/source_sound_dma_storage_tu15_generated.cpp
        src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_codec_initialization_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_codec_initialization_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage halo2_source_tu15_settings nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_codec_initialization_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_native_codec_initialization
        COMMAND native_tu15_sound_codec_initialization_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-codec-initialization/reference-v2/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-codec-initialization-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-codec-initialization-report.json)
    set_tests_properties(original_tu15_native_codec_initialization PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_codec_foreign_destructor
        COMMAND "${Python3_EXECUTABLE}" -c
        "import subprocess, sys; result = subprocess.run(sys.argv[1:]); sys.exit(0 if result.returncode == 77 else 1)"
        $<TARGET_FILE:native_tu15_sound_codec_initialization_tests> --foreign-destructor-probe
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-codec-initialization/reference-v2/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-codec-foreign-destructor-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-codec-foreign-destructor-unused-report.json)
    set_tests_properties(native_tu15_sound_codec_foreign_destructor PROPERTIES TIMEOUT 60)
endif()
