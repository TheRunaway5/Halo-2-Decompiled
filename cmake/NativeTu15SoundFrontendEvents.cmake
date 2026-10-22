target_sources(halo2_systems PRIVATE src/audio/runtime/source_runtime_sound_frontend_events.cpp
    src/audio/source_sound_frontend_events_tu15_generated.cpp
    src/audio/source_sound_frontend_dpc_tu15_generated.cpp src/audio/source_sound_pending_aggregate_tu15.cpp
    src/audio/native_tu15_sound_frontend_dpc_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_frontend_events_tests
        tests/native_tu15_sound_frontend_events_tests.cpp
        src/audio/runtime/source_runtime_sound_frontend_events.cpp src/audio/source_sound_frontend_events_tu15_generated.cpp
        src/audio/source_sound_frontend_dpc_tu15_generated.cpp src/audio/source_sound_pending_aggregate_tu15.cpp
        src/audio/runtime/source_runtime_sound_reset_resources.cpp src/audio/source_sound_reset_tu15_generated.cpp
        src/audio/runtime/source_runtime_sound_dma_resources.cpp src/audio/source_sound_dma_storage_tu15_generated.cpp
        src/runtime/source_runtime_host_physical_memory.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_frontend_events_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_frontend_events_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_frontend_events_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_native_sound_frontend_events
        COMMAND native_tu15_sound_frontend_events_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-frontend-events/original-final/report.json)
    set_tests_properties(original_tu15_native_sound_frontend_events PROPERTIES TIMEOUT 60)
endif()
