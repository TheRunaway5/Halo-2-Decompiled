# Native TU1.5 frame/gameplay owners of group F
# (docs/native_replacement/FRAME_MISC_OWNERS.md): seeded comparison against
# the pinned translated bodies with genuine child stand-ins, census and
# verify (record/replay) modes, sanitizer replay and mutation screens.
if(BUILD_TESTING)
    add_test(NAME native_tu15_two_bone_solve
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_two_bone_solve_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-two-bone-solve.json")
    set_tests_properties(native_tu15_two_bone_solve PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_polygon_intersection
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_polygon_intersection_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-polygon-intersection.json")
    set_tests_properties(native_tu15_polygon_intersection PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_bitmap_residency
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bitmap_residency_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bitmap-residency.json")
    set_tests_properties(native_tu15_bitmap_residency PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_e4dc0_contact_transition
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_physical_result_contact_e4dc0_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-e4dc0-contact-transition.json")
    set_tests_properties(native_tu15_e4dc0_contact_transition PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_e51d6_physical_result_decay
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_physical_result_decay_e51d6_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-e51d6-physical-result-decay.json")
    set_tests_properties(native_tu15_e51d6_physical_result_decay PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_e4b81_physical_result_classification
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_physical_result_classification_e4b81_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-e4b81-physical-result-classification.json")
    set_tests_properties(native_tu15_e4b81_physical_result_classification PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_e4a40_final_branches
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_physical_result_final_branches_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-e4a40-final-branches.json")
    set_tests_properties(native_tu15_e4a40_final_branches PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_physical_result_activity
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_physical_result_activity_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-physical-result-activity.json")
    set_tests_properties(native_tu15_physical_result_activity PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_death_cycle_opening
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_death_cycle_opening_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-death-cycle-opening.json")
    set_tests_properties(native_tu15_death_cycle_opening PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_death_cycle_activity
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_death_cycle_activity_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-death-cycle-activity.json")
    set_tests_properties(native_tu15_death_cycle_activity PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_death_cycle_expiration
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_death_cycle_expiration_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-death-cycle-expiration.json")
    set_tests_properties(native_tu15_death_cycle_expiration PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_death_cycle_motion
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_death_cycle_motion_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-death-cycle-motion.json")
    set_tests_properties(native_tu15_death_cycle_motion PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_death_cycle_motion_records_10387c
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_death_cycle_motion_records_10387c_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-death-cycle-motion-records-10387c.json")
    set_tests_properties(native_tu15_death_cycle_motion_records_10387c PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_death_cycle_object_timer
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_death_cycle_object_timer_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-death-cycle-object-timer.json")
    set_tests_properties(native_tu15_death_cycle_object_timer PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_death_cycle_record_blend
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_death_cycle_record_blend_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-death-cycle-record-blend.json")
    set_tests_properties(native_tu15_death_cycle_record_blend PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_weapon_animation_selection
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_weapon_animation_selection_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-weapon-animation-selection.json")
    set_tests_properties(native_tu15_weapon_animation_selection PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_animation_request
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_animation_request_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-animation-request.json")
    set_tests_properties(native_tu15_animation_request PROPERTIES TIMEOUT 2400)
    foreach(owner IN ITEMS 1306b0 d7230 1eee80 41970 133a70 153ba0 131610 14d200 bd460 1ae2b0 1ae090 3d7e0 16b240 170730 1d30f0 1e5250 333f0 15c4e0 3f3e0 bcfe0 17faf0 180520 144bf0 179a40 1a8e50 1902e0 b1720 17fb60 1d2f10 17beb0 165860 157fe0 e9530 10e1f0 125790 1e1390 116060 aa200 aa270 b9390 1751b0 e65a0
        106230 1d9f50 144cb0 bac30 10e340 be7e0 bda50 1ce810 130d20 145b70 e0500 c0700 1eedb0 18bbc0 c93d0
        c92c0 1e1620 110fc0 1e1920 11fa50 dfd20 1a8f10 175300
        175280 175240 1752c0 b16c0 17c5c0 1ae500 212c70 30c40 11e060 d2330 105950
        10dde0 10b0f0 1d3f90 16e290 b8d90 bbff0 caaa0
        14fbc0 ce230 cfb40 13f620 11e780 e3310 15dd10
        bc330 bbd50 15c7f0 df820 111d10
        c5250 17c330 c9590 1e5d60 ba910
        d2b00 1062a0 1cd620 11e600 152e00 106040 115ff0 1e15d0 c6e50 15b470 1f0f90 17e100 161560
        1cee60 1059c0 144210 1452f0 ba7e0 b92c0 d3a80 31edf0 33210c 161530 1d4140 116480 e32c0 cb440
        172200 bcf70 111f00 140ec0 105fd0 c8c20 113c70 1e5160
        1e5df0 1cf070 1cefb0
        1e5a70 bbf20
        1d4e50 1fa110 102910
        1e5f20 1e7810 212d40
        1e5e40 1521b0 152d60
        c8cf0 1be90
        2a0100 2db80 1d3930 1dc610 bc190 10c5d0 dfe30 1e5a50 335b0 150990 25bc70 226b0
        189f20 158450 25bcc0 d2b60 1d9f10 43980 12f0c0 118760 1d6430 304c50 1582f0
        18a240 171ff0 20700 1f33c0 c5170 31500 10db40 2e9750
        243510 16e7f0 2e3250 158340 1d9080 1d9100 3d2b0 1e5360 244f80
        168d76 168ded 158790 161610 13da89 1540d0 15b620
        1ae5e0 15b2d0 154d00 13e670 11ef70 1262f0 1ae6f0 b95d0
        c9010 1dc680 1157d0 153360 2e2e30 1edda0 1ceb70 c3960 140cf0 c3a20 be2b0 15cab0
        118120 4b150 c3820 1617a0 1a6830
        1718c0 15b740 e53e0 e52d0 113b70 14fdc0
        17cd10 1ce950 ba630 1cec60 e06e0
        1cf700 15b4d0 1459b0 15be80 1181f0 c8e70 c8390 d35c0
        1601b0 160360 160570 164890 1d5970 cdcf0 69ad0 1afa90 e71a0
        1d45a0 191130 be3b0 175380 144910 1af970 1d39c0 111b10 16bbc0
        16bda0 1cebd0 185820 13b610 16e510 13dc44 1d8b90 1d9190 2e9140
        315190 2f20f0 1ddea0 168ca1 164ef0 164750 17bd40 13b6f0 154270 15e870 e79c0 1ae7f0 1adc90 1770c0 245060 c7dd0 113190 1f8710 17c370 1618b0 17ead0 14d790 cd4a0 1de8e0 1538b0 1700e0 14d410 1115d0 1adf60 1139d0 15d890 d2bd0 4a7b0
        4ac20 1efd60 1ddbf0 cad60 1e1010 19ae70 1ce8d0 e05d0
        106160 1ee3d0 2e56a0 1e14f0
        116280 2e4350 24ffe0 10dfa0 13de70 b9460)
        add_test(NAME native_tu15_frame_misc_${owner}
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_frame_misc_tu15.py"
            --owner ${owner} --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-frame-misc-${owner}.json" --jobs 2)
        set_tests_properties(native_tu15_frame_misc_${owner} PROPERTIES TIMEOUT 2400)
    endforeach()
    add_test(NAME native_tu15_collision_pair_2f88d0
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_collision_pair_2f88d0.py"
        --compiler "${CMAKE_CXX_COMPILER}")
    set_tests_properties(native_tu15_collision_pair_2f88d0 PROPERTIES TIMEOUT 2400)
    foreach(owner IN ITEMS collision_cache_309060 collision_cache_305560_images
        contact_cache_update_30cd00 mopp_quantized_aabb_30a1e0 world_position_transform_2e4690)
        add_test(NAME native_tu15_${owner}
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_${owner}.py"
            --compiler "${CMAKE_CXX_COMPILER}")
        set_tests_properties(native_tu15_${owner} PROPERTIES TIMEOUT 2400)
    endforeach()
    add_test(NAME native_tu15_particle_draw_entry
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_draw_entry_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-draw-entry.json")
    set_tests_properties(native_tu15_particle_draw_entry PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_update_entry
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_update_entry_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-update-entry.json")
    set_tests_properties(native_tu15_damage_update_entry PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_update_return
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_update_return_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-update-return.json")
    set_tests_properties(native_tu15_damage_update_return PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_update_sections
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_update_sections_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-update-sections.json")
    set_tests_properties(native_tu15_damage_update_sections PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_update_notification
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_update_notification_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-update-notification.json")
    set_tests_properties(native_tu15_damage_update_notification PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_update_vitality_gate
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_update_vitality_gate_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-update-vitality-gate.json")
    set_tests_properties(native_tu15_damage_update_vitality_gate PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_update_flag_route
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_update_flag_route_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-update-flag-route.json")
    set_tests_properties(native_tu15_damage_update_flag_route PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_update_amounts
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_update_amounts_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-update-amounts.json")
    set_tests_properties(native_tu15_damage_update_amounts PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_update_shield_recovery
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_update_shield_recovery_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-update-shield-recovery.json")
    set_tests_properties(native_tu15_damage_update_shield_recovery PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_damage_descriptor_refresh
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_response_routes_tu15.py"
        --section descriptor --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-descriptor-refresh.json")
    set_tests_properties(native_tu15_damage_descriptor_refresh PROPERTIES TIMEOUT 2400)
    foreach(stage IN ITEMS due amount secondary)
        add_test(NAME native_tu15_damage_response_${stage}
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_response_routes_tu15.py"
            --section ${stage} --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-damage-response-${stage}.json")
        set_tests_properties(native_tu15_damage_response_${stage} PROPERTIES TIMEOUT 2400)
    endforeach()
    add_test(NAME native_tu15_campaign_fog_stages
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_campaign_fog_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-campaign-fog-stages.json")
    set_tests_properties(native_tu15_campaign_fog_stages PROPERTIES TIMEOUT 2400)
    if(EXISTS "${HALO2_TU15_GAMEPLAY_XBE}")
        add_test(NAME native_tu15_particle_observer_lod
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_observer_lod_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-observer-lod.json")
        set_tests_properties(native_tu15_particle_observer_lod PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_location_emitters
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_location_emitters_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-location-emitters.json")
        set_tests_properties(native_tu15_particle_location_emitters PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_location_entry
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_location_entry_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-location-entry.json")
        set_tests_properties(native_tu15_particle_location_entry PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_emitter_transform
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_emitter_transform_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-emitter-transform.json")
        set_tests_properties(native_tu15_emitter_transform PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_system_update
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_system_update_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-system-update.json")
        set_tests_properties(native_tu15_particle_system_update PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_system_attachment
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_system_attachment_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-system-attachment.json")
        set_tests_properties(native_tu15_particle_system_attachment PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_system_selection
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_system_selection_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-system-selection.json")
        set_tests_properties(native_tu15_particle_system_selection PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_system_contact_basis
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_system_contact_basis_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-system-contact-basis.json")
        set_tests_properties(native_tu15_particle_system_contact_basis PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_system_velocity_basis
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_system_velocity_basis_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-system-velocity-basis.json")
        set_tests_properties(native_tu15_particle_system_velocity_basis PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_drain_counts
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_drain_counts_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-drain-counts.json")
        set_tests_properties(native_tu15_particle_drain_counts PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_physics_shapes
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_physics_shapes_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-physics-shapes.json")
        set_tests_properties(native_tu15_particle_physics_shapes PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_contact_walk
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_contact_walk_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-contact-walk.json")
        set_tests_properties(native_tu15_particle_contact_walk PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_contact_flags
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_contact_flags_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-contact-flags.json")
        set_tests_properties(native_tu15_particle_contact_flags PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_contact_query
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_contact_query_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-contact-query.json")
        set_tests_properties(native_tu15_particle_contact_query PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_contact_response
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_contact_response_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-contact-response.json")
        set_tests_properties(native_tu15_particle_contact_response PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_contact_actions
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_contact_actions_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-contact-actions.json")
        set_tests_properties(native_tu15_particle_contact_actions PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_location_update
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_location_update_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-location-update.json")
        set_tests_properties(native_tu15_particle_location_update PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_emitter_emission
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_emitter_emission_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-emitter-emission.json")
        set_tests_properties(native_tu15_emitter_emission PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_location_prepare
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_location_prepare_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-location-prepare.json")
        set_tests_properties(native_tu15_particle_location_prepare PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_velocity_properties
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_velocity_properties_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-velocity-properties.json")
        set_tests_properties(native_tu15_particle_velocity_properties PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_velocity_direction
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_velocity_direction_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-velocity-direction.json")
        set_tests_properties(native_tu15_particle_velocity_direction PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_velocity_angles
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_velocity_angles_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-velocity-angles.json")
        set_tests_properties(native_tu15_particle_velocity_angles PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_velocity_remaining
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_velocity_remaining_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-velocity-remaining.json")
        set_tests_properties(native_tu15_particle_velocity_remaining PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_observer_lens
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_observer_lens_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-observer-lens.json")
        set_tests_properties(native_tu15_particle_observer_lens PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_light_shape_written
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${HALO2_RENDERER_ORACLE_PYTHON}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_light_shape_written_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}")
        set_tests_properties(native_tu15_light_shape_written PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_emitter_orientation
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_emitter_orientation_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-emitter-orientation.json")
        set_tests_properties(native_tu15_emitter_orientation PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_birth_prefix
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_birth_prefix_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-birth-prefix.json")
        set_tests_properties(native_tu15_particle_birth_prefix PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_birth_placement
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_birth_placement_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-birth-placement.json")
        set_tests_properties(native_tu15_particle_birth_placement PROPERTIES TIMEOUT 2400)
        add_test(NAME native_tu15_particle_birth_interface
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_birth_interface_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-particle-birth-interface.json")
        set_tests_properties(native_tu15_particle_birth_interface PROPERTIES TIMEOUT 2400)
    endif()
endif()
