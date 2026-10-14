# Same retained bank and native resource owners for the TU1.5 live world.
target_sources(halo2_systems PRIVATE src/physics_live_world_tu15.cpp src/source_runtime_cache_tags_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_runtime_resources.cpp)
target_sources(halo2_systems PRIVATE src/multiplayer_source_devices_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_boot_smc_video_mode.cpp)
target_sources(halo2_systems PRIVATE src/source_object_velocity_read_only_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_runtime_dsp_section.cpp src/source_runtime_sound_output.cpp src/source_runtime_sound_sdk_resources.cpp src/source_sound_sdk_pool.cpp)
target_link_libraries(halo2_systems PRIVATE OpenSSL::Crypto)
target_sources(halo2_physics_world_runtime PRIVATE src/physics_live_support_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_object_sound_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_player_departure_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_physics_deactivation_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_collision_instance_storage_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_collision_query_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_vehicle_occupancy_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_combat_action_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_effect_particle_frame_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_effect_particle_draw_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_ai_actor_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_ai_frame_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_contrail_geometry_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_sandbox_special_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_sound_startup_tu15_generated.cpp src/source_sound_sdk_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_cache_bitmap_view_tu15.cpp)
if(BUILD_TESTING)
    set(HALO2_TU15_AI_FRAME_CAPTURE_DIRECTORY "" CACHE PATH "Owned original TU1.5 AI frame and hardware replay")
    add_executable(ai_frame_tu15_tests tools/replay_native_ai_frame_tu15.cpp)
    target_link_libraries(ai_frame_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_ai_frame COMMAND ai_frame_tu15_tests
        "${HALO2_TU15_AI_FRAME_CAPTURE_DIRECTORY}" 0x1d0670
        "${CMAKE_CURRENT_BINARY_DIR}/original-ai-frame-native")
    set_tests_properties(original_tu15_ai_frame PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set(HALO2_TU15_BOOT_SMC_CAPTURE_DIRECTORY "" CACHE PATH "Observed owned original boot SMC transaction")
    add_executable(source_sound_audio_mode_tu15_tests tests/source_sound_audio_mode_tu15_tests.cpp)
    target_link_libraries(source_sound_audio_mode_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_audio_mode COMMAND source_sound_audio_mode_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${HALO2_TU15_BOOT_SMC_CAPTURE_DIRECTORY}" "${HALO2_TU15_SOUND_USER_EEPROM}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-sound-audio-mode")
    set_tests_properties(native_tu15_sound_audio_mode PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(contrail_geometry_tu15_tests tests/contrail_geometry_tu15_tests.cpp)
    target_link_libraries(contrail_geometry_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_contrail_geometry COMMAND contrail_geometry_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/contrail_geometry_tu15_original.json")
    add_test(NAME original_tu15_contrail_frame COMMAND contrail_geometry_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/contrail_frame_tu15_original.json")
    set(HALO2_TU15_PARTICLE_FRAME_CAPTURE_DIRECTORY "" CACHE PATH "Owned original charge-particle frame captures")
    add_executable(effect_particle_frame_tu15_tests tools/replay_native_effect_particle_tu15.cpp)
    target_link_libraries(effect_particle_frame_tu15_tests PRIVATE halo2_systems)
    foreach(frame_case charge-particle-original-after-stop-v2 charge-particle-original-after-stop-frame2-v2)
        add_test(NAME original_tu15_${frame_case} COMMAND effect_particle_frame_tu15_tests
            "${HALO2_TU15_PARTICLE_FRAME_CAPTURE_DIRECTORY}/${frame_case}" 0x179df0
            "${CMAKE_CURRENT_BINARY_DIR}/${frame_case}-native")
        set_tests_properties(original_tu15_${frame_case} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    foreach(init_case "effect-frame-audio-ctor-original-v1|0x22c440" "effect-frame-audio-reset-original-v1|0x22c480"
            "particle-visibility-original-v1|0x3f3e0" "particle-visible-frame-original-v1|0x179df0")
        string(REPLACE "|" ";" init_fields "${init_case}")
        list(GET init_fields 0 init_name)
        list(GET init_fields 1 init_entry)
        add_test(NAME original_tu15_${init_name} COMMAND effect_particle_frame_tu15_tests
            "${HALO2_TU15_PARTICLE_FRAME_CAPTURE_DIRECTORY}/${init_name}" "${init_entry}"
            "${CMAKE_CURRENT_BINARY_DIR}/${init_name}-native")
        set_tests_properties(original_tu15_${init_name} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    set(HALO2_TU15_SOUND_STARTUP_CAPTURE_DIRECTORY "" CACHE PATH "Owned unmodified TU1.5 sound startup capture")
    set(HALO2_TU15_SOUND_STARTUP_ORACLE_REPORT "" CACHE FILEPATH "Accepted independent original sound CPU replay")
    set(HALO2_TU15_DSP_SECTION_REPORT "" CACHE FILEPATH "Owned original TU1.5 DSP section reference capture")
    set(HALO2_TU15_SOUND_DEVICE_CAPTURE_DIRECTORY "" CACHE PATH "Owned unmodified TU1.5 sound driver capture")
    set(HALO2_TU15_SOUND_SDK_CAPTURE_DIRECTORY "" CACHE PATH "Owned unmodified TU1.5 SDK factory capture")
    set(HALO2_TU15_SOUND_POOL_RULES_CAPTURE_DIRECTORY "" CACHE PATH "Owned kernel pool rules observed during unmodified TU1.5 construction")
    set(HALO2_TU15_SOUND_AUDIO_CAPTURE_DIRECTORY "" CACHE PATH "Owned unmodified TU1.5 audio query capture")
    set(HALO2_TU15_SOUND_USER_EEPROM "" CACHE FILEPATH "Actual owned EEPROM resource for audio query acceptance")
    add_executable(source_runtime_sound_audio_settings_tests tests/source_runtime_sound_audio_settings_tests.cpp)
    target_link_libraries(source_runtime_sound_audio_settings_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_audio_settings COMMAND source_runtime_sound_audio_settings_tests
        "${HALO2_TU15_SOUND_SDK_CAPTURE_DIRECTORY}" "${HALO2_TU15_SOUND_AUDIO_CAPTURE_DIRECTORY}"
        "${HALO2_TU15_SOUND_USER_EEPROM}" "${CMAKE_CURRENT_BINARY_DIR}/native-sound-audio-settings")
    set_tests_properties(native_tu15_sound_audio_settings PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_sound_sdk_pool_tests tests/source_sound_sdk_pool_tests.cpp)
    target_link_libraries(source_sound_sdk_pool_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_sdk_pool_capacity COMMAND source_sound_sdk_pool_tests
        "${HALO2_TU15_SOUND_SDK_CAPTURE_DIRECTORY}" "${HALO2_TU15_SOUND_POOL_RULES_CAPTURE_DIRECTORY}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-sound-sdk-pool")
    set_tests_properties(native_tu15_sound_sdk_pool_capacity PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_runtime_sound_sdk_tests tests/source_runtime_sound_sdk_tests.cpp)
    target_link_libraries(source_runtime_sound_sdk_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_sdk_resources COMMAND source_runtime_sound_sdk_tests
        "${HALO2_TU15_SOUND_SDK_CAPTURE_DIRECTORY}" "${CMAKE_CURRENT_BINARY_DIR}/native-sound-sdk-prefix")
    set_tests_properties(native_tu15_sound_sdk_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_runtime_sound_output_tests tests/source_runtime_sound_output_tests.cpp)
    target_link_libraries(source_runtime_sound_output_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_output_resources COMMAND source_runtime_sound_output_tests
        "${HALO2_TU15_SOUND_DEVICE_CAPTURE_DIRECTORY}" "${CMAKE_CURRENT_BINARY_DIR}/native-sound-output-prefix")
    set_tests_properties(native_tu15_sound_output_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_runtime_dsp_section_tests tests/source_runtime_dsp_section_tests.cpp)
    target_link_libraries(source_runtime_dsp_section_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_dsp_section_resources COMMAND source_runtime_dsp_section_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${HALO2_TU15_DSP_SECTION_REPORT}" "${HALO2_TU15_SOUND_STARTUP_CAPTURE_DIRECTORY}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-dsp-section-prefix")
    set_tests_properties(native_tu15_dsp_section_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_sound_startup_tu15_tests tests/source_sound_startup_tu15_tests.cpp)
    target_link_libraries(source_sound_startup_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_startup_cpu COMMAND source_sound_startup_tu15_tests
        "${HALO2_TU15_SOUND_STARTUP_CAPTURE_DIRECTORY}" "${HALO2_TU15_SOUND_STARTUP_ORACLE_REPORT}")
    set_tests_properties(native_tu15_sound_startup_cpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(light_fade_curves_tu15_tests tests/light_fade_curves_tu15_tests.cpp)
    target_link_libraries(light_fade_curves_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_light_fade_curves COMMAND light_fade_curves_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/light_fade_curves_tu15_original.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    add_executable(physics_live_world_continuation_tests tests/physics_live_world_continuation_tests.cpp)
    target_link_libraries(physics_live_world_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_physics_live_world_continuation COMMAND physics_live_world_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_physics_live_world_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(decal_pass_tu15_tests tests/decal_pass_tu15_tests.cpp)
    target_link_libraries(decal_pass_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_decal_pass COMMAND decal_pass_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/decal_pass_tu15_original.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    add_executable(cache_vertex_view_tu15_tests tests/cache_vertex_view_tu15_tests.cpp)
    target_link_libraries(cache_vertex_view_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_cache_vertex_view COMMAND cache_vertex_view_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cache_vertex_view_tu15_original.json")
    add_executable(native_grenade_input_tests tests/native_grenade_input_tests.cpp)
    target_link_libraries(native_grenade_input_tests PRIVATE halo2_systems)
    add_test(NAME native_grenade_selector_input COMMAND native_grenade_input_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    add_executable(grenade_selection_tu15_tests tests/grenade_selection_tu15_tests.cpp src/inventory.cpp)
    target_include_directories(grenade_selection_tu15_tests PRIVATE include)
    target_link_libraries(grenade_selection_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_grenade_selection COMMAND grenade_selection_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/grenade_selection_tu15_original.json")
    add_executable(sandbox_special_tu15_tests tests/sandbox_special_tu15_tests.cpp)
    target_link_libraries(sandbox_special_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_sandbox_special COMMAND sandbox_special_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sandbox_special_tu15_original.json")
    set_tests_properties(original_tu15_sandbox_special PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_sandbox_primary COMMAND sandbox_special_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sandbox_primary_tu15_original.json")
    set_tests_properties(original_tu15_sandbox_primary PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(particle_collision_admission_tu15_tests tests/particle_collision_admission_tu15_tests.cpp)
    target_include_directories(particle_collision_admission_tu15_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(particle_collision_admission_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_particle_collision_admission COMMAND particle_collision_admission_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/particle_collision_admission_tu15_original.jsonl")
    add_test(NAME original_tu15_effect_event_timeline COMMAND source_effect_event_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_event_tu15_original.jsonl")
    add_executable(particle_effect_color_tu15_tests tests/particle_effect_color_tu15_tests.cpp)
    target_include_directories(particle_effect_color_tu15_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(particle_effect_color_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_particle_effect_color COMMAND particle_effect_color_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/particle_effect_color_tu15_original.json")
    set_tests_properties(original_tu15_particle_effect_color PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(capture_source_melee_contact tools/capture_source_melee_contact.cpp)
    add_executable(capture_source_locomotion_tu15 tools/capture_source_locomotion_tu15.cpp)
    add_executable(capture_source_render_pose_tu15 tools/capture_source_render_pose_tu15.cpp)
    target_link_libraries(capture_source_render_pose_tu15 PRIVATE halo2_systems)
    add_executable(capture_source_weapon_effect_events_tu15 tools/capture_source_weapon_effect_events_tu15.cpp)
    target_link_libraries(capture_source_weapon_effect_events_tu15 PRIVATE halo2_systems)
    add_test(NAME native_tu15_unmapped_source_weapon_effects COMMAND capture_source_weapon_effect_events_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/source-unmapped-weapon-effects" 5 unmapped)
    set_tests_properties(native_tu15_unmapped_source_weapon_effects PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME native_tu15_source_weapon_effect_locations_lan COMMAND capture_source_weapon_effect_events_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/source-network-weapon-effects" 5 network)
    set_tests_properties(native_tu15_source_weapon_effect_locations_lan PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME native_tu15_source_weapon_effect_events COMMAND capture_source_weapon_effect_events_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/source-weapon-effect-events")
    set_tests_properties(native_tu15_source_weapon_effect_events PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    target_link_libraries(capture_source_locomotion_tu15 PRIVATE halo2_systems)
    add_test(NAME native_tu15_elite_magnum_render_pose COMMAND capture_source_render_pose_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/source-elite-magnum-render-pose" elite 4)
    set_tests_properties(native_tu15_elite_magnum_render_pose PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    foreach(species human elite)
        add_test(NAME native_tu15_${species}_render_pose COMMAND capture_source_render_pose_tu15
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            "${CMAKE_CURRENT_BINARY_DIR}/source-${species}-render-pose" ${species} 5)
        set_tests_properties(native_tu15_${species}_render_pose PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME native_tu15_${species}_locomotion COMMAND capture_source_locomotion_tu15
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            "${CMAKE_CURRENT_BINARY_DIR}/source-${species}-locomotion" ${species})
        set_tests_properties(native_tu15_${species}_locomotion PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    target_link_libraries(capture_source_melee_contact PRIVATE halo2_systems)
    add_test(NAME native_tu15_plasma_decal_lifecycle COMMAND capture_source_melee_contact
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/source-plasma-decal-lifecycle" plasma-cache)
    set_tests_properties(native_tu15_plasma_decal_lifecycle PROPERTIES TIMEOUT 120)
    add_executable(capture_source_weapon_cycle_tu15 tools/capture_source_weapon_cycle_tu15.cpp)
    target_link_libraries(capture_source_weapon_cycle_tu15 PRIVATE halo2_systems)
    add_executable(capture_source_fp_runtime_tu15 tools/capture_source_weapon_cycle_tu15.cpp src/multiplayer_runtime.cpp)
    target_compile_definitions(capture_source_fp_runtime_tu15 PRIVATE HALO2_CAPTURE_SOURCE_FP_RUNTIME=1)
    target_link_libraries(capture_source_fp_runtime_tu15 PRIVATE halo2_renderer halo2_systems SDL2::SDL2)
    add_test(NAME native_tu15_scheduled_weapon_effects COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-scheduled-effects" first-person-scheduled-effects)
    set_tests_properties(native_tu15_scheduled_weapon_effects PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_sound_submit_events COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-sound-submit-acceptance" first-person-events)
    set_tests_properties(native_tu15_sound_submit_events PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_first_person_render_models COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-first-person-render-acceptance" first-person-render)
    set_tests_properties(native_tu15_first_person_render_models PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_primary_weapon_cycle COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-primary-acceptance")
    add_test(NAME native_tu15_primary_weapon_reload COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-reload-acceptance" reload)
    set_tests_properties(native_tu15_primary_weapon_cycle native_tu15_primary_weapon_reload
        PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_first_person_weapon_cycle COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-first-person-acceptance" first-person)
    set_tests_properties(native_tu15_first_person_weapon_cycle PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_first_person_player_birth COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-first-person-birth-acceptance" first-person-start)
    set_tests_properties(native_tu15_first_person_player_birth PROPERTIES TIMEOUT 180)
    if(UNIX AND NOT APPLE)
        target_link_options(capture_source_melee_contact PRIVATE -Wl,--strip-debug)
    endif()
    add_executable(engine_team_admission_tu15_tests tests/engine_team_admission_tu15_tests.cpp)
    target_link_libraries(engine_team_admission_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_engine_team_admission COMMAND engine_team_admission_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/engine_team_admission_tu15_original.json")
    set_tests_properties(original_tu15_engine_team_admission PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_engine_team_reset COMMAND engine_team_admission_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/engine_team_reset_tu15_original.json")
    set_tests_properties(original_tu15_engine_team_reset PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(combat_action_tu15_tests tests/combat_action_tu15_tests.cpp)
    target_link_libraries(combat_action_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_combat_action COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/combat_action_tu15_original.json")
    add_test(NAME original_tu15_decal_lifetime COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/decal_lifetime_tu15_original.json")
    add_test(NAME original_tu15_texture_cache_frame COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/texture_cache_frame_tu15_original.json")
    add_test(NAME original_tu15_effect_location_resolver COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_location_resolver_tu15_original.json")
    set_tests_properties(original_tu15_combat_action PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_cache_bitmap_view COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/combat_cache_bitmap_view_tu15_original.json")
    set_tests_properties(original_tu15_cache_bitmap_view PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(tu15_grenade_weapon_gate_tests tests/tu15_grenade_weapon_gate_tests.cpp)
    target_link_libraries(tu15_grenade_weapon_gate_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_grenade_weapon_gate COMMAND tu15_grenade_weapon_gate_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_grenade_weapon_gate_original.json")
    set_tests_properties(original_tu15_grenade_weapon_gate PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_device_script_runtime_tu15_tests tests/source_device_script_runtime_tu15_tests.cpp)
    target_link_libraries(source_device_script_runtime_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_source_device_script_tu15 COMMAND source_device_script_runtime_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map")
    set_tests_properties(native_source_device_script_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME native_source_wheel_power_tu15 COMMAND source_device_script_runtime_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map" --wheel-power)
    set_tests_properties(native_source_wheel_power_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(physics_live_world_tu15_tests tests/physics_live_world_tu15_tests.cpp)
    target_link_libraries(physics_live_world_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_physics_live_world_tu15 COMMAND physics_live_world_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_physics_live_world_tu15 PROPERTIES TIMEOUT 120)
endif()

target_sources(halo2_physics_world_runtime PRIVATE src/source_projectile_effect_observer.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_effect_world_locations.cpp)
target_link_libraries(halo2_physics_world_runtime PUBLIC halo2_raw_datum_pool_identity)
target_sources(halo2_systems PRIVATE src/source_projectile_presentation.cpp)
target_sources(halo2_systems PRIVATE src/source_combat_attachments.cpp)
