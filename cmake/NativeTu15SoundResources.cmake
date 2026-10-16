set(native_tu15_sound_resource_sources
    src/source_runtime_sound_dma_resources.cpp
    src/source_runtime_sound_reset_resources.cpp
    src/source_sound_dma_storage_tu15_generated.cpp
    src/source_sound_reset_tu15_generated.cpp
    src/source_sound_hardware_storage_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE ${native_tu15_sound_resource_sources})
target_link_libraries(halo2_systems PUBLIC halo2_sound_platform)
if(BUILD_TESTING)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE ${native_tu15_sound_resource_sources}
            src/source_runtime_sound_sdk_resources.cpp src/source_sound_sdk_pool.cpp)
        target_link_libraries(${target} PRIVATE halo2_sound_platform halo2_source_tu15_settings)
    endforeach()
    add_executable(native_tu15_sound_dma_storage_tests tests/native_tu15_sound_dma_storage_tests.cpp
        src/source_runtime_sound_dma_resources.cpp src/source_sound_dma_storage_tu15_generated.cpp
        src/source_runtime_host_physical_memory.cpp src/source_runtime_memory.cpp)
    add_executable(native_tu15_sound_reset_tests tests/native_tu15_sound_reset_tests.cpp
        src/source_runtime_sound_reset_resources.cpp src/source_sound_reset_tu15_generated.cpp
        src/source_runtime_memory.cpp)
    foreach(target native_tu15_sound_dma_storage_tests native_tu15_sound_reset_tests)
        target_include_directories(${target} PRIVATE src)
        target_link_libraries(${target} PRIVATE halo2_sound_platform halo2_physics_world_runtime
            halo2_snapshot_storage nlohmann_json::nlohmann_json)
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    endforeach()
    add_test(NAME original_tu15_sound_dma_storage_constructors COMMAND native_tu15_sound_dma_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-dma/original/report.json)
    add_test(NAME original_tu15_native_sound_reset COMMAND native_tu15_sound_reset_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-reset/original/report.json)
    set_tests_properties(original_tu15_sound_dma_storage_constructors original_tu15_native_sound_reset
        PROPERTIES TIMEOUT 60)
    foreach(input none virtual-four)
        if(input STREQUAL none)
            set(suffix "")
        else()
            set(suffix _controller)
        endif()
        add_test(NAME native_tu15_sound_sdk${suffix}_startup COMMAND native_tu15_graphics_startup_tests
            --frontier sound-sdk --input-devices ${input})
        set_tests_properties(native_tu15_sound_sdk${suffix}_startup
            PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endforeach()
endif()
