target_sources(halo2_systems PRIVATE src/native_tu15_sound_sdk_prefix_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_sdk_prefix_tests
        tests/native_tu15_sound_sdk_prefix_tests.cpp
        src/native_tu15_audio_preference.cpp src/native_tu15_sound_sdk_prefix_data.cpp
        src/source_runtime_sound_sdk_resources.cpp src/source_sound_sdk_pool.cpp
        src/source_runtime_sound_reset_resources.cpp src/source_sound_reset_tu15_generated.cpp
        src/source_sound_hardware_storage_tu15_generated.cpp src/source_sound_dma_storage_tu15_generated.cpp
        src/source_runtime_sound_dma_resources.cpp src/source_runtime_host_physical_memory.cpp
        src/source_sound_startup_tu15_generated.cpp src/source_startup_sound_storage_tu15_generated.cpp
        src/native_tu15_startup_sound_storage_data.cpp src/source_render_object_startup_tu15_generated.cpp
        src/source_runtime_host_storage.cpp src/source_runtime_host_virtual_memory.cpp src/physics_pool_host_storage.cpp
        src/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_sdk_prefix_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_sdk_prefix_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage halo2_source_tu15_settings
        nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_sdk_prefix_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_native_sound_sdk_prefix
        COMMAND native_tu15_sound_sdk_prefix_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-sdk-prefix/original-prefix-v2/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-sdk-prefix-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-sdk-prefix-report.json)
    set_tests_properties(original_tu15_native_sound_sdk_prefix PROPERTIES TIMEOUT 60)
    add_test(NAME original_tu15_native_sound_driver_sdk_reset_prefix
        COMMAND native_tu15_sound_sdk_prefix_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-sdk-prefix/original-driver-reset-prefix/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-driver-sdk-prefix-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-driver-sdk-prefix-report.json)
    set_tests_properties(original_tu15_native_sound_driver_sdk_reset_prefix PROPERTIES TIMEOUT 60)
    add_test(NAME original_tu15_native_sound_driver_storage_prefix
        COMMAND native_tu15_sound_sdk_prefix_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-sdk-prefix/original-driver-storage-prefix/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-driver-storage-prefix-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-driver-storage-prefix-report.json)
    set_tests_properties(original_tu15_native_sound_driver_storage_prefix PROPERTIES TIMEOUT 60)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE src/native_tu15_sound_sdk_prefix_data.cpp)
    endforeach()
endif()
