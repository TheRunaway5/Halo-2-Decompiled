set(native_tu15_sound_resource_sources
    src/audio/runtime/source_runtime_sound_dma_resources.cpp
    src/audio/runtime/source_runtime_sound_reset_resources.cpp
    src/audio/source_sound_dma_storage_tu15_generated.cpp
    src/audio/source_sound_reset_tu15_generated.cpp
    src/audio/source_sound_hardware_storage_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE ${native_tu15_sound_resource_sources})
target_link_libraries(halo2_systems PUBLIC halo2_sound_platform)
if(BUILD_TESTING)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE ${native_tu15_sound_resource_sources}
            src/audio/runtime/source_runtime_sound_sdk_resources.cpp src/audio/source_sound_sdk_pool.cpp)
        target_link_libraries(${target} PRIVATE halo2_sound_platform halo2_source_tu15_settings)
    endforeach()
    add_executable(native_tu15_sound_dma_storage_tests tests/native_tu15_sound_dma_storage_tests.cpp
        src/audio/runtime/source_runtime_sound_dma_resources.cpp src/audio/source_sound_dma_storage_tu15_generated.cpp
        src/runtime/source_runtime_host_physical_memory.cpp src/runtime/source_runtime_memory.cpp)
    add_executable(native_tu15_sound_reset_tests tests/native_tu15_sound_reset_tests.cpp
        src/audio/runtime/source_runtime_sound_reset_resources.cpp src/audio/source_sound_reset_tu15_generated.cpp
        src/runtime/source_runtime_memory.cpp)
    add_executable(native_tu15_sound_gp_pool_tests tests/native_tu15_sound_gp_pool_tests.cpp
        src/audio/runtime/source_runtime_sound_dma_resources.cpp src/runtime/source_runtime_host_physical_memory.cpp
        src/runtime/source_runtime_memory.cpp src/audio/source_sound_sdk_tu15_generated.cpp
        src/audio/dsp/source_sound_dsp_image_tu15_generated.cpp)
    target_include_directories(native_tu15_sound_gp_pool_tests PRIVATE include src)
    target_link_libraries(native_tu15_sound_gp_pool_tests PRIVATE halo2_sound_platform halo2_snapshot_storage)
    target_compile_options(native_tu15_sound_gp_pool_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME native_tu15_sound_gp_pool_pages COMMAND native_tu15_sound_gp_pool_tests)
    set_tests_properties(native_tu15_sound_gp_pool_pages PROPERTIES TIMEOUT 60)
    add_executable(native_tu15_sound_gp_scratch_tests tests/native_tu15_sound_gp_scratch_tests.cpp
        src/audio/runtime/source_runtime_sound_dma_resources.cpp src/runtime/source_runtime_host_physical_memory.cpp
        src/runtime/source_runtime_memory.cpp src/audio/source_sound_dma_storage_tu15_generated.cpp
        src/audio/source_sound_sdk_tu15_generated.cpp src/audio/dsp/source_sound_dsp_image_tu15_generated.cpp)
    target_include_directories(native_tu15_sound_gp_scratch_tests PRIVATE include src)
    target_link_libraries(native_tu15_sound_gp_scratch_tests PRIVATE halo2_sound_platform
        halo2_snapshot_storage halo2_sound_gp_image_tu15)
    target_compile_options(native_tu15_sound_gp_scratch_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME native_tu15_sound_gp_scratch_publication COMMAND native_tu15_sound_gp_scratch_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_tu15_sound_gp_scratch_publication PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    add_executable(native_tu15_sound_ep_scratch_tests tests/native_tu15_sound_ep_scratch_tests.cpp
        src/audio/runtime/source_runtime_sound_dma_resources.cpp src/runtime/source_runtime_host_physical_memory.cpp
        src/audio/runtime/source_runtime_sound_sdk_resources.cpp src/audio/source_sound_sdk_pool.cpp
        src/runtime/source_runtime_image.cpp src/runtime/source_runtime_memory.cpp src/platform/source_tu15_settings.cpp
        src/audio/source_sound_dma_storage_tu15_generated.cpp src/audio/source_sound_sdk_tu15_generated.cpp
        src/audio/dsp/source_sound_dsp_image_tu15_generated.cpp src/audio/source_sound_ac97_output_tu15_generated.cpp)
    target_include_directories(native_tu15_sound_ep_scratch_tests PRIVATE include src)
    target_link_libraries(native_tu15_sound_ep_scratch_tests PRIVATE halo2_sound_platform halo2_snapshot_storage)
    target_compile_options(native_tu15_sound_ep_scratch_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME native_tu15_sound_ep_scratch_initializer
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py
        --output-root @HALO2_TEST_SCRATCH@/guard --timeout 60 --
        $<TARGET_FILE:native_tu15_sound_ep_scratch_tests>
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_tu15_sound_ep_scratch_initializer PROPERTIES TIMEOUT 90 SKIP_RETURN_CODE 77)
    add_executable(native_tu15_sound_gp_upload_tests tests/native_tu15_sound_gp_upload_tests.cpp
        src/audio/runtime/source_runtime_sound_dma_resources.cpp src/runtime/source_runtime_host_physical_memory.cpp
        src/audio/runtime/source_runtime_sound_sdk_resources.cpp src/audio/source_sound_sdk_pool.cpp
        src/runtime/source_runtime_dsp_section.cpp src/runtime/source_runtime_image.cpp src/runtime/source_runtime_memory.cpp
        src/platform/source_tu15_settings.cpp
        src/audio/source_sound_dma_storage_tu15_generated.cpp src/audio/source_sound_sdk_tu15_generated.cpp
        src/audio/dsp/source_sound_dsp_image_tu15_generated.cpp)
    target_include_directories(native_tu15_sound_gp_upload_tests PRIVATE include src)
    target_link_libraries(native_tu15_sound_gp_upload_tests PRIVATE halo2_sound_platform
        halo2_snapshot_storage nlohmann_json::nlohmann_json OpenSSL::Crypto)
    target_compile_options(native_tu15_sound_gp_upload_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_native_tu15_sound_gp_upload_suffix
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_sound_gp_upload_tu15.py
        --native $<TARGET_FILE:native_tu15_sound_gp_upload_tests>
        --xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_upload_suffix_tu15
        --output-root ${CMAKE_CURRENT_BINARY_DIR}/native-gp-upload-results)
    set_tests_properties(original_native_tu15_sound_gp_upload_suffix PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    option(HALO2_BUILD_GP_UPLOAD_CONSUMER_TESTS "Build actual native SGE/GP bootstrap composition with declared peripheral context" OFF)
    if(HALO2_BUILD_GP_UPLOAD_CONSUMER_TESTS)
        get_target_property(_halo2_gp_upload_sources native_tu15_sound_gp_upload_tests SOURCES)
        add_executable(native_tu15_sound_gp_consumer_tests ${_halo2_gp_upload_sources})
        target_compile_definitions(native_tu15_sound_gp_consumer_tests PRIVATE HALO2_TEST_GP_CONSUMER=1)
        target_include_directories(native_tu15_sound_gp_consumer_tests PRIVATE include src)
        target_link_libraries(native_tu15_sound_gp_consumer_tests PRIVATE halo2_sound_platform
            halo2_snapshot_storage nlohmann_json::nlohmann_json OpenSSL::Crypto
            halo2_sound_gp_cpu_tu15 halo2_sound_gp_dma_tu15)
        target_compile_options(native_tu15_sound_gp_consumer_tests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
        add_test(NAME original_native_tu15_sound_gp_consumer
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_sound_gp_upload_tu15.py
            --native $<TARGET_FILE:native_tu15_sound_gp_consumer_tests>
            --xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_upload_suffix_tu15
            --gp-context ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_dma_tu15/reference-context.json
            --output-root ${CMAKE_CURRENT_BINARY_DIR}/native-gp-consumer-results)
        set_tests_properties(original_native_tu15_sound_gp_consumer PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
        add_test(NAME original_native_tu15_sound_gp_upload_return
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_sound_gp_upload_tu15.py
            --native $<TARGET_FILE:native_tu15_sound_gp_consumer_tests>
            --xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_upload_suffix_tu15
            --gp-context ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_dma_tu15/reference-context.json
            --return-reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_upload_return_tu15
            --output-root ${CMAKE_CURRENT_BINARY_DIR}/native-gp-return-results)
        set_tests_properties(original_native_tu15_sound_gp_upload_return PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    endif()
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
