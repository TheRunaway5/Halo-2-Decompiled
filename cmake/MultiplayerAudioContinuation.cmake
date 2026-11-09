# Actual Core/script/announcer playback owners. These local transactions do
# not certify complete foreground/native readiness or enable migration.
target_sources(halo2_systems PRIVATE src/audio/source_loop_combined_publication_tu15_generated.cpp
    src/audio/source_loop_combined_routing_tu15_generated.cpp)
if(MSVC)
    set_source_files_properties(src/audio/source_loop_combined_publication_tu15_generated.cpp
        src/audio/source_loop_combined_routing_tu15_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/audio/source_loop_combined_publication_tu15_generated.cpp
        src/audio/source_loop_combined_routing_tu15_generated.cpp PROPERTIES COMPILE_OPTIONS
        "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(native_loop_combined_publication_tu15_tests
        tests/native_loop_combined_publication_tu15_tests.cpp
        src/audio/native_tu15_sound_kernels_gain.cpp src/audio/source_loop_combined_publication_tu15_generated.cpp)
    target_include_directories(native_loop_combined_publication_tu15_tests PRIVATE include src tests)
    target_compile_features(native_loop_combined_publication_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(native_loop_combined_publication_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm)
    if(NOT MSVC)
        target_compile_options(native_loop_combined_publication_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME native_loop_combined_publication_tu15 COMMAND native_loop_combined_publication_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_combined_publication_tu15.json")
    add_executable(native_loop_combined_routing_tu15_tests
        tests/native_loop_combined_routing_tu15_tests.cpp
        src/audio/native_tu15_sound_kernels_gain.cpp src/audio/source_loop_combined_routing_tu15_generated.cpp)
    target_include_directories(native_loop_combined_routing_tu15_tests PRIVATE include src tests)
    target_compile_features(native_loop_combined_routing_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(native_loop_combined_routing_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm)
    if(NOT MSVC)
        target_compile_options(native_loop_combined_routing_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME native_loop_combined_routing_tu15 COMMAND native_loop_combined_routing_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_combined_routing_tu15.json")
    add_executable(source_loop_combined_driver_tu15_tests
        tests/source_loop_combined_driver_tu15_tests.cpp
        src/audio/source_sound_gain_envelope_tu15.cpp src/audio/sound_spatial.cpp)
    target_include_directories(source_loop_combined_driver_tu15_tests PRIVATE include)
    target_compile_features(source_loop_combined_driver_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(source_loop_combined_driver_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm)
    if(NOT MSVC)
        target_compile_options(source_loop_combined_driver_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME original_loop_combined_driver_tu15 COMMAND source_loop_combined_driver_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_combined_driver_tu15.json"
        "${PROJECT_SOURCE_DIR}/tests/fixtures/sound_gain_envelope_tu15.json")
    set_tests_properties(original_loop_combined_driver_tu15 PROPERTIES SKIP_RETURN_CODE 77)
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
            src/audio/source_loop_track_control_tu15.cpp
            src/audio/source_loop_detail_control_tu15.cpp
            src/audio/source_loop_phased_output_tu15.cpp
            src/audio/source_loop_track_update_tu15.cpp
            src/audio/source_loop_track_transition_tu15.cpp
            src/audio/source_loop_combined_publication_tu15_generated.cpp
            src/audio/source_loop_combined_routing_tu15_generated.cpp
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
            src/audio/source_announcer_assets_tu15.cpp
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
            # Native publication checks the actual immutable resource owner.
            # Some Cache preparation fixtures already compile this owner.
            get_target_property(_audio_continuation_sources ${target} SOURCES)
            if(NOT "src/runtime/source_runtime_resources.cpp" IN_LIST _audio_continuation_sources)
                target_sources(${target} PRIVATE src/runtime/source_runtime_resources.cpp)
            endif()
        else()
            target_link_libraries(${target} PRIVATE halo2_systems)
        endif()
        target_compile_features(${target} PRIVATE cxx_std_20)
    endfunction()
    if(TARGET original_sound_preparation_cache_tests)
        halo2_link_audio_continuation(original_sound_preparation_cache_tests)
    endif()
    add_executable(original_sound_metadata_publication_tests tests/original_sound_metadata_publication_tests.cpp)
    halo2_link_audio_continuation(original_sound_metadata_publication_tests)
    set(HALO2_ORIGINAL_SOUND_METADATA_MAP "" CACHE FILEPATH "Produced original-sound map for immutable client playback validation")
    add_test(NAME original_sound_metadata_publication COMMAND original_sound_metadata_publication_tests
        "${HALO2_ORIGINAL_SOUND_METADATA_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/original-sound-metadata-publication.json")
    set_tests_properties(original_sound_metadata_publication PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(original_sound_world_tu15_tests tests/original_sound_world_tu15_tests.cpp)
    target_link_libraries(original_sound_world_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json OpenSSL::Crypto)
    set(HALO2_ORIGINAL_SOUND_WORLD_ADMISSION_MODE "" CACHE STRING "Original sound World test admission path: countdown or direct")
    set(HALO2_ORIGINAL_SOUND_WORLD_BASELINE_MAP "" CACHE FILEPATH "Identical pre-catalog Cache for original sound World admission")
    set(_original_world_admission_args)
    if(HALO2_ORIGINAL_SOUND_WORLD_ADMISSION_MODE)
        list(APPEND _original_world_admission_args "${HALO2_ORIGINAL_SOUND_WORLD_ADMISSION_MODE}" "${HALO2_ORIGINAL_SOUND_WORLD_BASELINE_MAP}")
    endif()
    add_test(NAME original_sound_world_tu15 COMMAND original_sound_world_tu15_tests
        "${HALO2_MCC_SOUND_TIMING_MAP}" "${HALO2_ORIGINAL_SOUND_CONTEXT}"
        "${HALO2_ORIGINAL_SOUND_CATALOG}" "${HALO2_ORIGINAL_SOUND_CATALOG_SHA256}"
        "@HALO2_TEST_SCRATCH@/world" "${CMAKE_CURRENT_BINARY_DIR}/original-sound-world-tu15.json" ${_original_world_admission_args})
    set_tests_properties(original_sound_world_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600
        ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache;SDL_AUDIODRIVER=dummy")
    add_executable(source_loop_voice_ring_tu15_tests tests/source_loop_voice_ring_tu15_tests.cpp)
    halo2_link_audio_continuation(source_loop_voice_ring_tu15_tests)
    add_test(NAME original_loop_voice_ring_tu15 COMMAND source_loop_voice_ring_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_voice_ring_tu15.json")
    add_executable(source_loop_combined_request_tu15_tests tests/source_loop_combined_request_tu15_tests.cpp)
    halo2_link_audio_continuation(source_loop_combined_request_tu15_tests)
    add_test(NAME original_loop_combined_request_tu15 COMMAND source_loop_combined_request_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_combined_request_tu15.json")
    add_executable(vehicle_destruction_loop_playback_tu15_tests tests/vehicle_destruction_loop_playback_tu15_tests.cpp)
    halo2_link_audio_continuation(vehicle_destruction_loop_playback_tu15_tests)
    set(HALO2_DESTRUCTION_LOOP_TEST_MAP "" CACHE FILEPATH "Prepared MCC level for destruction loop PCM qualification")
    add_executable(source_loop_combined_listener_tu15_tests tests/source_loop_combined_listener_tu15_tests.cpp)
    halo2_link_audio_continuation(source_loop_combined_listener_tu15_tests)
    add_test(NAME original_loop_combined_listener_tu15 COMMAND source_loop_combined_listener_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_combined_listener_tu15.json")
    add_test(NAME native_combined_listener_binding COMMAND source_loop_combined_listener_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_combined_listener_tu15.json" "${HALO2_DESTRUCTION_LOOP_TEST_MAP}")
    set_tests_properties(original_loop_combined_listener_tu15 native_combined_listener_binding
        PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    set(HALO2_DESTRUCTION_LOOP_INVENTORY "" CACHE FILEPATH "Explicit authenticated destruction-loop inventory")
    add_test(NAME native_vehicle_destruction_loops COMMAND vehicle_destruction_loop_playback_tu15_tests
        "${HALO2_DESTRUCTION_LOOP_TEST_MAP}" "${HALO2_DESTRUCTION_LOOP_INVENTORY}"
        "${CMAKE_CURRENT_BINARY_DIR}/destruction-loops.json")
    set_tests_properties(native_vehicle_destruction_loops PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_test(NAME native_vehicle_destruction_alternates COMMAND vehicle_destruction_loop_playback_tu15_tests
        "${HALO2_DESTRUCTION_LOOP_TEST_MAP}" "${HALO2_DESTRUCTION_LOOP_INVENTORY}"
        "${CMAKE_CURRENT_BINARY_DIR}/destruction-alternates.json" alternates)
    set_tests_properties(native_vehicle_destruction_alternates PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(vehicle_destruction_pcm_resources_tu15_tests tests/vehicle_destruction_audio_playback_tu15_tests.cpp)
    halo2_link_audio_continuation(vehicle_destruction_pcm_resources_tu15_tests)
    set(HALO2_DESTRUCTION_PCM_PLAN "" CACHE FILEPATH "Explicit authored destruction sound resource plan")
    set(HALO2_DESTRUCTION_ACTUAL_PCM_PLAN "" CACHE FILEPATH "Pinned captured destruction sound requests for native PCM replay")
    if(HALO2_DESTRUCTION_ACTUAL_PCM_PLAN)
        add_test(NAME native_vehicle_destruction_actual_pcm_requests COMMAND vehicle_destruction_pcm_resources_tu15_tests
            --actual-requests "${HALO2_DESTRUCTION_ACTUAL_PCM_PLAN}" "${CMAKE_CURRENT_BINARY_DIR}/destruction-actual-audio.json")
        set_tests_properties(native_vehicle_destruction_actual_pcm_requests PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
            ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    endif()
    add_test(NAME native_vehicle_destruction_pcm_resources COMMAND vehicle_destruction_pcm_resources_tu15_tests
        "${HALO2_DESTRUCTION_PCM_PLAN}" "${CMAKE_CURRENT_BINARY_DIR}/destruction-audio-native.json")
    set_tests_properties(native_vehicle_destruction_pcm_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(multiplayer_audio_loop_phase_tests tests/multiplayer_audio_loop_phase_tests.cpp)
    halo2_link_audio_continuation(multiplayer_audio_loop_phase_tests)
    add_executable(multiplayer_audio_chunk_queue_tests tests/multiplayer_audio_chunk_queue_tests.cpp)
    halo2_link_audio_continuation(multiplayer_audio_chunk_queue_tests)
    set(HALO2_ORIGINAL_SOUND_CHUNK_PLAYBACK_MAP "" CACHE FILEPATH "Original-catalog Coagulation map for actual chunk output")
    add_test(NAME native_original_sound_chunk_playback COMMAND multiplayer_audio_chunk_queue_tests
        "${HALO2_ORIGINAL_SOUND_CHUNK_PLAYBACK_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/original-sound-chunk-playback.json")
    set_tests_properties(native_original_sound_chunk_playback PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300 RUN_SERIAL TRUE
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    set(HALO2_MCC_LOOP_PHASE_MAP "" CACHE FILEPATH "Explicit prepared Coagulation map for actual Banshee phase PCM")
    add_test(NAME native_mcc_loop_phase_packets COMMAND multiplayer_audio_loop_phase_tests
        "${HALO2_MCC_LOOP_PHASE_MAP}")
    set_tests_properties(native_mcc_loop_phase_packets PROPERTIES SKIP_RETURN_CODE 77
        TIMEOUT 120 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy"
        LABELS "audio-continuation;component")
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
