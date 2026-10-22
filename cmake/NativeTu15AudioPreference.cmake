target_sources(halo2_systems PRIVATE src/audio/native_tu15_audio_preference.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_audio_preference_tests tests/native_tu15_audio_preference_tests.cpp
        src/audio/native_tu15_audio_preference.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_audio_preference_tests PRIVATE src)
    target_link_libraries(native_tu15_audio_preference_tests PRIVATE halo2_sound_platform
        halo2_physics_world_runtime halo2_snapshot_storage nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_audio_preference_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME native_tu15_audio_explicit_preference
        COMMAND native_tu15_audio_preference_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-audio-startup-seams/original-explicit-preference-abi/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-audio-preference-private
        ${CMAKE_CURRENT_BINARY_DIR}/native-audio-preference-report.json)
    set_tests_properties(native_tu15_audio_explicit_preference PROPERTIES TIMEOUT 60)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE src/audio/native_tu15_audio_preference.cpp)
        target_link_libraries(${target} PRIVATE halo2_sound_platform)
    endforeach()
endif()
