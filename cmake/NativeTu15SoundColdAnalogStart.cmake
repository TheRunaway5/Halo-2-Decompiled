# Genuine bounded cold analog source start/stop. Full processor/configuration
# and cold initializer 45 remain required.
if(BUILD_TESTING)
    get_target_property(_halo2_cold_analog_sources native_tu15_sound_codec_events_tests SOURCES)
    list(REMOVE_ITEM _halo2_cold_analog_sources tests/native_tu15_sound_codec_events_tests.cpp)
    add_executable(native_tu15_sound_cold_analog_start_tests
        tests/native_tu15_sound_cold_analog_start_tests.cpp
        src/audio/runtime/source_runtime_sound_pcm_prepared_range.cpp
        ${_halo2_cold_analog_sources})
    # ChannelReset's private Endpoint/Consumer definitions also host the
    # cold-start implementation. Older isolated targets need its real route
    # dependencies even when their test exercises only reset or prepared PCM.
    foreach(_halo2_cold_analog_target IN ITEMS
            native_tu15_sound_ac97_channel_reset_tests
            native_tu15_sound_pcm_buffers_tests
            native_tu15_sound_pcm_prepared_range_tests
            native_tu15_sound_ac97_page_release_tests)
        get_target_property(_halo2_cold_analog_existing ${_halo2_cold_analog_target} SOURCES)
        foreach(_halo2_cold_analog_source IN LISTS _halo2_cold_analog_sources)
            if(NOT _halo2_cold_analog_source IN_LIST _halo2_cold_analog_existing)
                target_sources(${_halo2_cold_analog_target} PRIVATE ${_halo2_cold_analog_source})
            endif()
        endforeach()
    endforeach()
    unset(_halo2_cold_analog_target)
    unset(_halo2_cold_analog_existing)
    unset(_halo2_cold_analog_source)
    unset(_halo2_cold_analog_sources)
    target_include_directories(native_tu15_sound_cold_analog_start_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_cold_analog_start_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage halo2_source_tu15_settings
        nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_cold_analog_start_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME native_tu15_sound_cold_analog_source_start
        COMMAND native_tu15_sound_cold_analog_start_tests
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-cold-analog-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-cold-analog-report.json)
    set_tests_properties(native_tu15_sound_cold_analog_source_start PROPERTIES TIMEOUT 60)
    add_test(NAME native_tu15_sound_cold_analog_foreign_destructor
        COMMAND "${Python3_EXECUTABLE}" -c
        "import subprocess, sys; result = subprocess.run(sys.argv[1:]); sys.exit(0 if result.returncode == 77 else 1)"
        $<TARGET_FILE:native_tu15_sound_cold_analog_start_tests> --foreign-destructor-probe
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-cold-analog-foreign-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-cold-analog-foreign-unused-report.json)
    set_tests_properties(native_tu15_sound_cold_analog_foreign_destructor PROPERTIES TIMEOUT 60)
endif()
