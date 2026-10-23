# Actual Core/script/announcer playback owners. These local transactions do
# not certify complete foreground/native readiness or enable migration.
if(BUILD_TESTING)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8
        AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$"
        AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
        # Compile this real owner closure once for both focused executables.
        # ELF section GC drops unused functions, never substitutes World or
        # Cache symbols. EXCLUDE_FROM_ALL preserves normal build storage policy.
        add_library(halo2_audio_continuation_owners OBJECT EXCLUDE_FROM_ALL
            src/audio/multiplayer_audio.cpp
            src/audio/audio.cpp
            src/ui/menu_music.cpp
            src/audio/sound_classes.cpp
            src/audio/sound_fade.cpp
            src/audio/vista_audio_decode.cpp
            src/audio/vista_audio_framing.cpp
            src/campaign/scripting/source_script_sound_clock_native64_tu15.cpp
            src/audio/mcc_sound_variation_provider.cpp
            src/audio/mcc_sound_variation_native_binding.cpp
            src/runtime/source_native_publication.cpp
            src/gameplay/multiplayer/multiplayer_object_runtime.cpp
            src/campaign/scripting/mcc_script_sound_world_access.cpp
            src/physics/physics_live_world_tu15.cpp
            src/runtime/source_runtime_memory.cpp
            src/runtime/source_runtime_snapshot_bytes.cpp
            src/runtime/source_execution_inputs.cpp
            src/runtime/source_execution_calls.cpp
            src/runtime/source_runtime_dsp_section.cpp
            src/audio/runtime/source_runtime_sound_sdk_resources.cpp
            src/rendering/effects/effect_audio_auxiliary.cpp
            src/ui/vista/native_vista_announcer.cpp
            src/audio/vista_announcer_assets.cpp
            src/audio/vista_announcer_events.cpp
            src/audio/vista_announcer_queue.cpp)
        target_compile_features(halo2_audio_continuation_owners PUBLIC cxx_std_20)
        target_compile_options(halo2_audio_continuation_owners PUBLIC
            -ffunction-sections -fdata-sections -fno-fast-math
            -ffp-contract=off -frounding-math)
        target_link_options(halo2_audio_continuation_owners INTERFACE
            "LINKER:--gc-sections")
        target_link_libraries(halo2_audio_continuation_owners PUBLIC
            halo2_cache halo2_sound_platform halo2_script_sound_clock_tu15
            Threads::Threads PkgConfig::AVCODEC)
        if(HALO2_SANITIZERS)
            target_compile_options(halo2_audio_continuation_owners PUBLIC
                -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(halo2_audio_continuation_owners INTERFACE
                -fsanitize=address,undefined)
        endif()
    endif()
    function(halo2_link_audio_continuation target)
        if(TARGET halo2_audio_continuation_owners)
            target_link_libraries(${target} PRIVATE halo2_audio_continuation_owners)
        else()
            target_link_libraries(${target} PRIVATE halo2_systems)
        endif()
        target_compile_features(${target} PRIVATE cxx_std_20)
    endfunction()
    add_executable(multiplayer_audio_continuation_tests
        tests/multiplayer_audio_continuation_tests.cpp)
    halo2_link_audio_continuation(multiplayer_audio_continuation_tests)
    add_executable(native_mcc_announcer_continuation_tests
        tests/native_mcc_announcer_continuation_tests.cpp)
    halo2_link_audio_continuation(native_mcc_announcer_continuation_tests)
    # This case constructs an actual MCC four-player native world, an encoded
    # authority cut, and all four Effects owners. Keep their real full closure.
    add_executable(native_mcc_damage_audio_continuation_tests
        tests/native_mcc_damage_audio_continuation_tests.cpp)
    target_compile_features(native_mcc_damage_audio_continuation_tests PRIVATE cxx_std_20)
    target_link_libraries(native_mcc_damage_audio_continuation_tests PRIVATE
        halo2_systems halo2_authority_codec OpenSSL::Crypto)
    add_test(NAME native_audio_core_owner_preflight
        COMMAND multiplayer_audio_continuation_tests --owner-preflight)
    set_tests_properties(native_audio_core_owner_preflight PROPERTIES
        TIMEOUT 30 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy"
        LABELS "audio-continuation;component")
    set(HALO2_MCC_CORE_AUDIO_TEST_MAP "" CACHE FILEPATH
        "Explicit imported MCC Lockout map for actual playback continuation")
    if(HALO2_MCC_CORE_AUDIO_TEST_MAP)
        add_test(NAME native_mcc_audio_core_enrollment
            COMMAND multiplayer_audio_continuation_tests --mcc
                "${HALO2_MCC_CORE_AUDIO_TEST_MAP}")
        add_test(NAME native_mcc_announcer_continuation
            COMMAND native_mcc_announcer_continuation_tests --mcc
                "${HALO2_MCC_CORE_AUDIO_TEST_MAP}")
        add_test(NAME native_mcc_damage_audio_continuation
            COMMAND native_mcc_damage_audio_continuation_tests --mcc
                "${HALO2_MCC_CORE_AUDIO_TEST_MAP}")
        set_tests_properties(native_mcc_audio_core_enrollment
            native_mcc_announcer_continuation PROPERTIES
            TIMEOUT 180 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy"
            LABELS "audio-continuation;mcc;component")
        set_tests_properties(native_mcc_damage_audio_continuation PROPERTIES
            TIMEOUT 300 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy"
            LABELS "audio-continuation;mcc;native-component")
    endif()
endif()
