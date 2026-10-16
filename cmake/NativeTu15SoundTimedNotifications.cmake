# Requires the reviewed timed-notification frontend/EventPort extension.
# Full source audio configuration and playing voice/PIO paths remain required.
target_sources(halo2_systems PRIVATE src/source_runtime_sound_timed_notifications.cpp
    src/source_sound_timed_notifications_tu15_generated.cpp src/native_tu15_sound_timed_notification_data.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(native_tu15_sound_timed_notifications_tests
        tests/native_tu15_sound_timed_notifications_tests.cpp
        src/source_runtime_sound_timed_notifications.cpp src/source_sound_timed_notifications_tu15_generated.cpp
        src/native_tu15_sound_timed_notification_data.cpp src/source_runtime_sound_event_port.cpp
        src/native_tu15_sound_event_port_data.cpp src/source_runtime_sound_frontend_events.cpp
        src/source_sound_frontend_events_tu15_generated.cpp src/source_sound_frontend_dpc_tu15_generated.cpp
        src/source_sound_pending_aggregate_tu15.cpp src/native_tu15_sound_frontend_dpc_data.cpp
        src/native_tu15_sound_sdk_prefix_data.cpp src/source_runtime_sound_reset_resources.cpp
        src/source_sound_reset_tu15_generated.cpp src/source_runtime_sound_dma_resources.cpp
        src/source_sound_dma_storage_tu15_generated.cpp src/source_runtime_host_physical_memory.cpp src/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_timed_notifications_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_timed_notifications_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_timed_notifications_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME native_tu15_sound_timed_notifications
        COMMAND native_tu15_sound_timed_notifications_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-platform-registration/original-registered/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-timed-notifications-report.json)
    add_test(NAME native_tu15_sound_timed_notifications_foreign_destructor
        COMMAND "${Python3_EXECUTABLE}" -c
        "import subprocess, sys; result = subprocess.run(sys.argv[1:]); sys.exit(0 if result.returncode == 77 else 1)"
        $<TARGET_FILE:native_tu15_sound_timed_notifications_tests> --foreign-destructor-probe
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-platform-registration/original-registered/report.json)
    set_tests_properties(native_tu15_sound_timed_notifications native_tu15_sound_timed_notifications_foreign_destructor PROPERTIES TIMEOUT 60)
endif()
