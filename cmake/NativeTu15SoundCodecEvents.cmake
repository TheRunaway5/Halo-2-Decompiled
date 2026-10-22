# Actual descriptor completion/registered shutdown component. No cold
# configuration bypass, source output-start or DSP PCM producer admission.
target_sources(halo2_systems PRIVATE src/audio/runtime/source_runtime_sound_codec_event_port.cpp
    src/audio/source_sound_codec_events_tu15_generated.cpp src/audio/native_tu15_sound_codec_event_data.cpp)
if(BUILD_TESTING)
    # ChannelReset's new exclusive consumer resolves actual PCM bindings.
    target_sources(native_tu15_sound_ac97_channel_reset_tests PRIVATE
        src/audio/runtime/source_runtime_sound_pcm_buffers.cpp)
    add_executable(native_tu15_sound_codec_events_tests
        tests/native_tu15_sound_codec_events_tests.cpp
        src/audio/runtime/source_runtime_sound_codec_event_port.cpp src/audio/native_tu15_sound_codec_event_data.cpp
        src/audio/source_sound_codec_events_tu15_generated.cpp src/audio/runtime/source_runtime_sound_codec_route.cpp
        src/audio/source_sound_codec_initialization_tu15_generated.cpp src/audio/native_tu15_sound_codec_initialization_data.cpp
        src/platform/source_process_shutdown.cpp src/audio/runtime/source_runtime_sound_ac97_page_release.cpp
        src/audio/source_sound_ac97_page_release_tu15_generated.cpp src/audio/runtime/source_runtime_sound_pcm_buffers.cpp
        src/audio/runtime/source_runtime_sound_ac97_channel_reset.cpp src/audio/source_sound_ac97_retire_tu15_generated.cpp
        src/audio/native_tu15_sound_ac97_retire_data.cpp src/audio/source_sound_ac97_output_tu15_generated.cpp
        src/audio/runtime/source_runtime_sound_ac97_output_pages.cpp src/audio/native_tu15_sound_ac97_output_data.cpp
        src/audio/runtime/source_runtime_sound_ep_output_resources.cpp src/audio/native_tu15_audio_preference.cpp
        src/audio/native_tu15_sound_sdk_prefix_data.cpp src/audio/runtime/source_runtime_sound_sdk_resources.cpp
        src/audio/source_sound_sdk_pool.cpp src/audio/runtime/source_runtime_sound_dma_resources.cpp
        src/audio/runtime/source_runtime_sound_reset_resources.cpp src/audio/source_sound_reset_tu15_generated.cpp
        src/audio/source_sound_hardware_storage_tu15_generated.cpp src/audio/source_sound_dma_storage_tu15_generated.cpp
        src/runtime/source_runtime_host_physical_memory.cpp src/audio/source_sound_sdk_tu15_generated.cpp
        src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_codec_events_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_codec_events_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage halo2_source_tu15_settings
        nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_codec_events_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME native_tu15_sound_codec_descriptor_events
        COMMAND native_tu15_sound_codec_events_tests
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-codec-events-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-codec-events-report.json)
    set_tests_properties(native_tu15_sound_codec_descriptor_events PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_codec_events_foreign_destructor
        COMMAND "${Python3_EXECUTABLE}" -c
        "import subprocess, sys; result = subprocess.run(sys.argv[1:]); sys.exit(0 if result.returncode == 77 else 1)"
        $<TARGET_FILE:native_tu15_sound_codec_events_tests> --foreign-destructor-probe
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-codec-events-foreign-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-codec-events-foreign-unused-report.json)
    set_tests_properties(native_tu15_sound_codec_events_foreign_destructor PROPERTIES TIMEOUT 60)
endif()
