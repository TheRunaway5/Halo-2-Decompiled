# Opt-in native MCC regressions for the reported online presentation failures.
# Localhost delivery and native GPU checks retain their component boundaries;
# these registrations do not assert WAN, campaign or Xbox framebuffer parity.
set(HALO2_MCC_ONLINE_PRESENTATION_MAP "" CACHE FILEPATH
    "Prepared MCC Lockout map for native online presentation regressions")
if(BUILD_TESTING)
    add_executable(source_world_pose_wire_tests tests/source_world_pose_wire_tests.cpp
        src/rendering/models/source_world_pose_wire.cpp)
    target_include_directories(source_world_pose_wire_tests PRIVATE include)
    add_test(NAME source_world_pose_wire COMMAND source_world_pose_wire_tests)
    set_tests_properties(source_world_pose_wire PROPERTIES TIMEOUT 30
        LABELS "online-presentation;native-component;network")
    add_executable(source_world_attachment_interpolation_tests
        tests/source_world_attachment_interpolation_tests.cpp)
    target_link_libraries(source_world_attachment_interpolation_tests PRIVATE halo2_systems)
    add_test(NAME source_world_attachment_interpolation COMMAND source_world_attachment_interpolation_tests)
    set_tests_properties(source_world_attachment_interpolation PROPERTIES TIMEOUT 30
        LABELS "online-presentation;native-component")
    add_executable(source_native_render_timeline_tests tests/source_native_render_timeline_tests.cpp)
    target_link_libraries(source_native_render_timeline_tests PRIVATE halo2_systems)
    add_test(NAME source_native_render_continuity COMMAND source_native_render_timeline_tests)
    set_tests_properties(source_native_render_continuity PROPERTIES TIMEOUT 30
        LABELS "online-presentation;native-component")
    add_executable(source_first_person_packet_transport_tests
        tests/source_first_person_packet_transport_tests.cpp)
    target_link_libraries(source_first_person_packet_transport_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_first_person_packet_transport COMMAND source_first_person_packet_transport_tests
        "${CMAKE_CURRENT_BINARY_DIR}/native-first-person-packet-transport.json")
    set_tests_properties(native_first_person_packet_transport PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
        LABELS "online-presentation;native-component;network")
    add_executable(native_player_camera_transport_tests tests/native_player_camera_transport_tests.cpp)
    target_link_libraries(native_player_camera_transport_tests PRIVATE halo2_systems)
    add_test(NAME native_player_camera_transport COMMAND native_player_camera_transport_tests)
    set_tests_properties(native_player_camera_transport PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
        LABELS "online-presentation;native-component;network")
    add_executable(source_object_loop_emitter_transport_tests
        tests/source_object_loop_emitter_transport_tests.cpp)
    target_link_libraries(source_object_loop_emitter_transport_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_object_loop_emitter_transport COMMAND source_object_loop_emitter_transport_tests
        "${CMAKE_CURRENT_BINARY_DIR}/native-object-loop-emitter-transport.json")
    set_tests_properties(native_object_loop_emitter_transport PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
        LABELS "online-presentation;native-component;network")
    add_executable(object_loop_online_delivery_tu15_tests
        tests/object_loop_online_delivery_tu15_tests.cpp)
    target_link_libraries(object_loop_online_delivery_tu15_tests PRIVATE
        halo2_systems OpenSSL::Crypto nlohmann_json::nlohmann_json)
    add_executable(pstencil_point_caster_identity_tests
        tests/pstencil_point_caster_identity_tests.cpp)
    target_link_libraries(pstencil_point_caster_identity_tests PRIVATE halo2_systems)
    add_executable(source_weapon_effect_retirement_tu15_tests
        tests/source_weapon_effect_retirement_tu15_tests.cpp)
    target_link_libraries(source_weapon_effect_retirement_tu15_tests PRIVATE halo2_systems)
    add_executable(native_first_person_pose_equivalence_tests
        tests/native_first_person_pose_equivalence_tests.cpp)
    target_link_libraries(native_first_person_pose_equivalence_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_executable(native_first_person_visibility_tu15_tests
        tests/native_first_person_visibility_tu15_tests.cpp)
    target_link_libraries(native_first_person_visibility_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_executable(native_body_grip_publication_tests
        tests/native_body_grip_publication_tests.cpp)
    target_link_libraries(native_body_grip_publication_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_executable(native_body_dual_class_tu15_tests
        tests/native_body_dual_class_tu15_tests.cpp)
    target_link_libraries(native_body_dual_class_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_executable(native_first_person_registry_tu15_tests
        tests/native_first_person_registry_tu15_tests.cpp)
    target_link_libraries(native_first_person_registry_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    if(UNIX)
        add_executable(native_dual_pose_submission_tu15_tests
            tests/native_dual_pose_submission_tu15_tests.cpp
            src/gameplay/multiplayer/multiplayer_runtime.cpp)
        target_link_libraries(native_dual_pose_submission_tu15_tests PRIVATE
            halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
        target_compile_definitions(native_dual_pose_submission_tu15_tests PRIVATE
            HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
        add_dependencies(native_dual_pose_submission_tu15_tests halo2_shaders)
        add_executable(native_first_person_split_submission_tu15_tests
            tests/native_dual_pose_submission_tu15_tests.cpp
            src/gameplay/multiplayer/multiplayer_runtime.cpp)
        target_link_libraries(native_first_person_split_submission_tu15_tests PRIVATE
            halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
        target_compile_definitions(native_first_person_split_submission_tu15_tests PRIVATE
            HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}" HALO2_NATIVE_FP_SPLIT_SUBMISSION=1)
        add_dependencies(native_first_person_split_submission_tu15_tests halo2_shaders)
    endif()
endif()
if(BUILD_TESTING AND HALO2_MCC_ONLINE_PRESENTATION_MAP)
    get_filename_component(online_presentation_maps "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" DIRECTORY)
    add_test(NAME native_mcc_body_grip_publication
        COMMAND native_body_grip_publication_tests
            "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
            "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_body_grip_publication.json")
    set_tests_properties(native_mcc_body_grip_publication PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE
        LABELS "online-presentation;mcc;native-component" ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_test(NAME native_mcc_body_grip_fractional
        COMMAND native_body_grip_publication_tests
            "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
            "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_body_grip_fractional.json" fractional)
    set_tests_properties(native_mcc_body_grip_fractional PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
        LABELS "online-presentation;mcc;native-component" ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_test(NAME native_mcc_pstencil_point_caster_identity
        COMMAND pstencil_point_caster_identity_tests "${HALO2_MCC_ONLINE_PRESENTATION_MAP}")
    set_tests_properties(native_mcc_pstencil_point_caster_identity PROPERTIES TIMEOUT 120
        LABELS "online-presentation;mcc;native-component")
    add_test(NAME native_mcc_weapon_effect_retirement
        COMMAND source_weapon_effect_retirement_tu15_tests
            "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}")
    set_tests_properties(native_mcc_weapon_effect_retirement PROPERTIES TIMEOUT 180
        LABELS "online-presentation;mcc;native-component")
    add_test(NAME native_mcc_dual_class_metadata COMMAND mcc_first_person_binding_tests
        "${HALO2_MCC_ONLINE_PRESENTATION_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_dual_class_metadata.json")
    set_tests_properties(native_mcc_dual_class_metadata PROPERTIES TIMEOUT 120
        LABELS "online-presentation;mcc;native-component")
    find_program(HALO2_PRESENTATION_XVFB_RUN xvfb-run)
    function(halo2_online_presentation_test name target)
        if(NOT TARGET ${target})
            return()
        endif()
        if(HALO2_PRESENTATION_XVFB_RUN)
            add_test(NAME ${name} COMMAND "${HALO2_PRESENTATION_XVFB_RUN}" -a
                -s "-screen 0 800x600x24" "$<TARGET_FILE:${target}>" ${ARGN})
        else()
            add_test(NAME ${name} COMMAND "$<TARGET_FILE:${target}>" ${ARGN})
        endif()
        set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600 RUN_SERIAL TRUE
            LABELS "online-presentation;mcc;native-component"
            ENVIRONMENT "SDL_AUDIODRIVER=dummy;HALO2_SOUND_ENTRY_CENSUS=1;XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache;HALO2_PIPELINE_CACHE_DIRECTORY=@HALO2_TEST_SCRATCH@/vulkan-cache;MESA_SHADER_CACHE_DIR=@HALO2_TEST_SCRATCH@/mesa-cache;__GL_SHADER_DISK_CACHE_PATH=@HALO2_TEST_SCRATCH@/nvidia-cache"
            ENVIRONMENT_MODIFICATION
                "HALO2_REFERENCE_SOUND_MANAGER=unset:;HALO2_SOUND_MANAGER_ORACLE=unset:")
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "SDL_VIDEODRIVER=x11")
        endif()
    endfunction()
    foreach(species human elite)
        if(species STREQUAL elite)
            set(representation 3)
        else()
            set(representation 2)
        endif()
        add_test(NAME native_mcc_fp_visibility_${species}
            COMMAND native_first_person_visibility_tu15_tests
                "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_fp_visibility_${species}.json"
                "${representation}")
        set_tests_properties(native_mcc_fp_visibility_${species} PROPERTIES
            TIMEOUT 180 RUN_SERIAL TRUE LABELS "online-presentation;mcc;native-component"
            ENVIRONMENT "SDL_AUDIODRIVER=dummy")
        foreach(fp_route local remote)
            set(registry_test native_mcc_fp_registry_${fp_route}_${species})
            add_test(NAME ${registry_test} COMMAND native_first_person_registry_tu15_tests
                "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                "${CMAKE_CURRENT_BINARY_DIR}/${registry_test}.json" "${representation}" "${fp_route}")
            set_tests_properties(${registry_test} PROPERTIES TIMEOUT 240 RUN_SERIAL TRUE
                LABELS "online-presentation;mcc;native-component" ENVIRONMENT "SDL_AUDIODRIVER=dummy")
            set(birth_test native_mcc_fp_birth_${fp_route}_${species})
            add_test(NAME ${birth_test} COMMAND native_first_person_registry_tu15_tests
                "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                "${CMAKE_CURRENT_BINARY_DIR}/${birth_test}.json" "${representation}" "${fp_route}" birth-only)
            set_tests_properties(${birth_test} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
                LABELS "online-presentation;mcc;native-component" ENVIRONMENT "SDL_AUDIODRIVER=dummy")
            if(fp_route STREQUAL remote)
                set(abi_test native_mcc_fp_caller_abi_${species})
                add_test(NAME ${abi_test} COMMAND native_first_person_registry_tu15_tests
                    "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                    "${CMAKE_CURRENT_BINARY_DIR}/${abi_test}.json" "${representation}" "${fp_route}" caller-abi-only)
                set_tests_properties(${abi_test} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
                    LABELS "online-presentation;mcc;native-component" ENVIRONMENT "SDL_AUDIODRIVER=dummy")
            endif()
        endforeach()
        add_test(NAME native_mcc_body_dual_class_${species}
            COMMAND native_body_dual_class_tu15_tests
                "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                "${representation}" "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_body_dual_class_${species}.json")
        set_tests_properties(native_mcc_body_dual_class_${species} PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE
            LABELS "online-presentation;mcc;native-component" ENVIRONMENT "SDL_AUDIODRIVER=dummy")
        if(TARGET native_dual_pose_submission_tu15_tests)
            foreach(route host joined)
                foreach(graphics original pstencil)
                    set(name native_mcc_dual_submission_${route}_${graphics}_${species})
                    halo2_online_presentation_test(${name} native_dual_pose_submission_tu15_tests
                        "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                        "${CMAKE_CURRENT_BINARY_DIR}/${name}.json" "${representation}" "${route}" "${graphics}")
                    if("${graphics}" STREQUAL "original")
                        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
                            "HALO2_ENABLE_GPU_SKINNING=1" "HALO2_VERIFY_GPU_SKINNING=1"
                            "HALO2_VERIFY_GPU_FIRST_PERSON_INPUTS=1")
                        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
                            "HALO2_REFERENCE_CPU_SKINNING=unset:")
                    endif()
                endforeach()
            endforeach()
        endif()
        if(TARGET native_first_person_split_submission_tu15_tests)
            foreach(graphics original pstencil)
                set(name native_mcc_fp_split_submission_${graphics}_${species})
                halo2_online_presentation_test(${name} native_first_person_split_submission_tu15_tests
                    "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                    "${CMAKE_CURRENT_BINARY_DIR}/${name}.json" "${representation}" "${graphics}")
                if("${graphics}" STREQUAL "original")
                    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
                        "HALO2_ENABLE_GPU_SKINNING=1" "HALO2_VERIFY_GPU_SKINNING=1"
                        "HALO2_VERIFY_GPU_FIRST_PERSON_INPUTS=1")
                    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
                        "HALO2_REFERENCE_CPU_SKINNING=unset:")
                endif()
            endforeach()
        endif()
        add_test(NAME native_mcc_fp_native_equivalence_${species}
            COMMAND native_first_person_pose_equivalence_tests
                "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_fp_native_equivalence_${species}.json"
                "${representation}")
        set_tests_properties(native_mcc_fp_native_equivalence_${species} PROPERTIES
            TIMEOUT 180 RUN_SERIAL TRUE LABELS "online-presentation;mcc;native-component"
            ENVIRONMENT "SDL_AUDIODRIVER=dummy;HALO2_SOUND_ENTRY_CENSUS=1")
        foreach(pose_mode extras dynamic native packet packet_mixed packet_single packet_render_order)
            if(pose_mode STREQUAL native)
                set(pose_label consumer)
            else()
                set(pose_label "${pose_mode}")
            endif()
            add_test(NAME native_mcc_fp_native_${pose_label}_${species}
                COMMAND native_first_person_pose_equivalence_tests
                    "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
                    "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_fp_native_${pose_label}_${species}.json"
                    "${representation}" "${pose_mode}")
            set_tests_properties(native_mcc_fp_native_${pose_label}_${species} PROPERTIES
                TIMEOUT 180 RUN_SERIAL TRUE LABELS "online-presentation;mcc;native-component"
                ENVIRONMENT "SDL_AUDIODRIVER=dummy;HALO2_SOUND_ENTRY_CENSUS=1")
        endforeach()
        foreach(route host_gpu host_pstencil joined)
            set(name native_mcc_dual_${route}_${species})
            halo2_online_presentation_test(${name} native_weapon_runtime_tests
                "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}" native-mcc)
            set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
                "HALO2_TEST_REPRESENTATION=${representation}"
                "HALO2_TEST_DUAL_HISTORY_REPORT=${CMAKE_CURRENT_BINARY_DIR}/${name}.json")
            if(route STREQUAL host_gpu)
                set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
                    "HALO2_TEST_DUAL_GPU=1" "HALO2_VERIFY_GPU_FIRST_PERSON_INPUTS=1"
                    "HALO2_VERIFY_GPU_SKINNING=1")
            elseif(route STREQUAL joined)
                set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "HALO2_TEST_NETWORK_DUAL=1")
                set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "HALO2_TEST_RENDER_MODE=pstencil")
            else()
                set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "HALO2_TEST_RENDER_MODE=pstencil")
            endif()
        endforeach()
    endforeach()
    foreach(route local joined)
        set(arguments --weapons --announcer)
        if(route STREQUAL joined)
            list(APPEND arguments --lan)
        endif()
        halo2_online_presentation_test(native_mcc_announcer_${route} multiplayer_audio_runtime_tests
            "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}" native-mcc ${arguments})
        set(name native_mcc_damage_voice_${route})
        halo2_online_presentation_test(${name} native_death_match_tu15_tests
            native-mcc "${HALO2_MCC_ONLINE_PRESENTATION_MAP}")
        if(TEST ${name})
            set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "HALO2_TU15_TEST_DAMAGE_VOICE=1")
            if(route STREQUAL joined)
                set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "HALO2_TU15_TEST_NETWORK=1")
            endif()
        endif()
    endforeach()
    halo2_online_presentation_test(native_mcc_player_eye_joined native_death_match_tu15_tests
        native-mcc "${HALO2_MCC_ONLINE_PRESENTATION_MAP}")
    if(TEST native_mcc_player_eye_joined)
        set_property(TEST native_mcc_player_eye_joined APPEND PROPERTY
            ENVIRONMENT "HALO2_TU15_TEST_NETWORK=1" "HALO2_TU15_TEST_PLAYER_EYE=1")
    endif()
    halo2_online_presentation_test(native_mcc_death_pose_rate native_death_remote_render_tu15_tests
        native-mcc "${HALO2_MCC_ONLINE_PRESENTATION_MAP}")
    if(TEST native_mcc_death_pose_rate)
    # Verify adjacent complete WORLD endpoints and exact fractional skinning;
    # budgeted delta-stream presentation has its own transport controls.
        set_property(TEST native_mcc_death_pose_rate APPEND PROPERTY ENVIRONMENT_MODIFICATION "H2_NET_PROFILE=set:complete")
        set_property(TEST native_mcc_death_pose_rate APPEND PROPERTY ENVIRONMENT
            "HALO2_DEATH_POSE_RATE=1" "HALO2_DEATH_AUDIO=1"
            "HALO2_DEATH_PHYSICS_REPORT=${CMAKE_CURRENT_BINARY_DIR}/native_mcc_death_physics.json")
    endif()
    halo2_online_presentation_test(native_mcc_scoreboard_mode_formats multiplayer_death_scoreboard_runtime_tests
        "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}" native-mcc)
    set_property(TEST native_mcc_scoreboard_mode_formats APPEND PROPERTY ENVIRONMENT "HALO2_SCOREBOARD_FORMAT_MATRIX=1")
    add_test(NAME native_mcc_scoreboard_source_phase COMMAND multiplayer_scoreboard_phase_tu15_tests
        "${HALO2_MCC_ONLINE_PRESENTATION_MAP}")
    set_tests_properties(native_mcc_scoreboard_source_phase PROPERTIES TIMEOUT 300
        LABELS "online-presentation;mcc;native-component")
    halo2_online_presentation_test(native_mcc_scoreboard_controllers multiplayer_scoreboard_controller_runtime_tests
        "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}" native-mcc)
    halo2_online_presentation_test(native_mcc_lift_audio object_loop_audio_runtime_tu15_tests
        "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
        "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_lift_audio.json" native-object-loop lift)
    halo2_online_presentation_test(native_mcc_lift_online object_loop_online_delivery_tu15_tests
        "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${online_presentation_maps}"
        "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_lift_online.json")
endif()
