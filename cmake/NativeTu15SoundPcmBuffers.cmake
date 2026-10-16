# Include after NativeTu15SoundAc97ChannelReset.cmake. This binds the exact
# default PCM source descriptor to actual retained EP DMA pages. It does not
# admit codec, channel start, DSP production or the complete initializer45.
target_sources(halo2_systems PRIVATE src/source_runtime_sound_pcm_buffers.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_pcm_buffers_tests
        tests/native_tu15_sound_pcm_buffers_tests.cpp src/source_runtime_sound_pcm_buffers.cpp
        src/source_runtime_sound_ac97_channel_reset.cpp src/source_sound_ac97_retire_tu15_generated.cpp
        src/native_tu15_sound_ac97_retire_data.cpp src/source_runtime_sound_ac97_output_pages.cpp
        src/native_tu15_sound_ac97_output_data.cpp src/source_sound_ac97_output_tu15_generated.cpp
        src/source_runtime_sound_ep_output_resources.cpp src/native_tu15_audio_preference.cpp
        src/native_tu15_sound_sdk_prefix_data.cpp src/source_runtime_sound_sdk_resources.cpp
        src/source_sound_sdk_pool.cpp src/source_runtime_sound_dma_resources.cpp
        src/source_runtime_host_physical_memory.cpp src/source_runtime_sound_reset_resources.cpp
        src/source_sound_reset_tu15_generated.cpp src/source_sound_hardware_storage_tu15_generated.cpp
        src/source_sound_dma_storage_tu15_generated.cpp src/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_pcm_buffers_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_pcm_buffers_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage halo2_source_tu15_settings
        nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_pcm_buffers_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_native_pcm_buffers
        COMMAND native_tu15_sound_pcm_buffers_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-pcm-buffers/reference/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-pcm-buffers-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-pcm-buffers-report.json)
    set_tests_properties(original_tu15_native_pcm_buffers PROPERTIES TIMEOUT 60)
endif()
