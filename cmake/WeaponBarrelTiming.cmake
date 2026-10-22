target_sources(halo2_systems PRIVATE src/gameplay/weapons/weapon_barrel_timing.cpp src/gameplay/weapons/weapon_energy.cpp src/gameplay/weapons/weapon_magazine.cpp src/gameplay/weapons/passive_weapon.cpp src/gameplay/weapons/weapon_hand_feedback.cpp src/ui/hud/weapon_hud.cpp src/gameplay/weapons/weapon_trigger.cpp src/gameplay/weapons/charged_weapon.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/weapon_hud_ownership.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/source_light_volume_draw.cpp)
add_executable(audit_weapon_muzzle_admission tools/audit_weapon_muzzle_admission.cpp)
target_link_libraries(audit_weapon_muzzle_admission PRIVATE halo2_systems)
if(MSVC)
  set_source_files_properties(src/gameplay/weapons/weapon_barrel_timing.cpp src/gameplay/weapons/weapon_energy.cpp src/gameplay/weapons/weapon_magazine.cpp src/gameplay/weapons/passive_weapon.cpp src/gameplay/weapons/weapon_hand_feedback.cpp src/ui/hud/weapon_hud.cpp src/gameplay/weapons/weapon_trigger.cpp src/gameplay/weapons/charged_weapon.cpp src/rendering/effects/source_light_volume_draw.cpp src/rendering/shader_animation.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(src/gameplay/weapons/weapon_barrel_timing.cpp src/gameplay/weapons/weapon_energy.cpp src/gameplay/weapons/weapon_magazine.cpp src/gameplay/weapons/passive_weapon.cpp src/gameplay/weapons/weapon_hand_feedback.cpp src/ui/hud/weapon_hud.cpp src/gameplay/weapons/weapon_trigger.cpp src/gameplay/weapons/charged_weapon.cpp src/rendering/effects/source_light_volume_draw.cpp src/rendering/shader_animation.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
# Dedicated progress launchers retain the original Xbox gameplay HUD.
foreach(weapon_launcher IN ITEMS halo2_weapons_launcher halo2_weapons_title_launcher)
  add_executable(${weapon_launcher} src/app/weapon_launcher.cpp)
  target_link_libraries(${weapon_launcher} PRIVATE SDL2::SDL2)
  target_compile_definitions(${weapon_launcher} PRIVATE
      HALO2_PYTHON_EXECUTABLE="${Python3_EXECUTABLE}"
      HALO2_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
      HALO2_RUNTIME_DIR="${CMAKE_CURRENT_BINARY_DIR}")
  add_dependencies(${weapon_launcher} halo2_native)
endforeach()
set_target_properties(halo2_weapons_launcher PROPERTIES OUTPUT_NAME "Halo2-Weapons-Launcher")
set_target_properties(halo2_weapons_title_launcher PROPERTIES OUTPUT_NAME "Halo2-Weapons-Title-Launcher")
target_compile_definitions(halo2_weapons_title_launcher PRIVATE HALO2_TITLE_FIRST=1)
if(BUILD_TESTING)
  add_executable(projectile_light_attachment_tests tests/projectile_light_attachment_tests.cpp)
  target_link_libraries(projectile_light_attachment_tests PRIVATE halo2_systems)
  add_test(NAME original_projectile_light_attachment COMMAND projectile_light_attachment_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/projectile_light_attachment_original.tsv)
  set_tests_properties(original_projectile_light_attachment PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(projectile_light_volume_tests tests/projectile_light_volume_tests.cpp)
  target_link_libraries(projectile_light_volume_tests PRIVATE halo2_cache)
  add_test(NAME original_projectile_light_volume COMMAND projectile_light_volume_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/projectile_light_volume_original.inputs.bin
      ${CMAKE_CURRENT_BINARY_DIR}/projectile-light-volume.native.bin
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/projectile_light_volume_original.expected.bin)
  add_executable(projectile_speed_tests tests/projectile_speed_tests.cpp)
  target_link_libraries(projectile_speed_tests PRIVATE halo2_systems)
  add_test(NAME original_projectile_speed COMMAND projectile_speed_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/projectile_speed_original.tsv)
  set_tests_properties(original_projectile_speed PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(charged_weapon_cycles_tests tests/charged_weapon_cycles_tests.cpp)
  target_link_libraries(charged_weapon_cycles_tests PRIVATE halo2_systems)
  add_test(NAME original_charged_weapon_cycles COMMAND charged_weapon_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/charged_weapon_cycles_original.tsv
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/charged_effect_state_original.tsv)
  set_tests_properties(original_charged_weapon_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_trigger_tests tests/weapon_trigger_tests.cpp)
  target_link_libraries(weapon_trigger_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_trigger COMMAND weapon_trigger_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_trigger_original.tsv
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_trigger_phase_original.tsv)
  set_tests_properties(original_weapon_trigger PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_hud_gameplay_tests tests/weapon_hud_gameplay_tests.cpp)
  target_link_libraries(weapon_hud_gameplay_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_hud_gameplay COMMAND weapon_hud_gameplay_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
  add_executable(weapon_hud_tests tests/weapon_hud_tests.cpp)
  target_link_libraries(weapon_hud_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_hud COMMAND weapon_hud_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_hud_original.tsv
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_reserve_original.tsv)
  set_tests_properties(original_weapon_hud PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_hand_feedback_tests tests/weapon_hand_feedback_tests.cpp)
  target_link_libraries(weapon_hand_feedback_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_hand_feedback COMMAND weapon_hand_feedback_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_hand_feedback_original.tsv)
  set_tests_properties(original_weapon_hand_feedback PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_unequip_cycles_tests tests/weapon_unequip_cycles_tests.cpp)
  target_link_libraries(weapon_unequip_cycles_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_unequip_cycles COMMAND weapon_unequip_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_unequip_cycles_original.tsv)
  set_tests_properties(original_weapon_unequip_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_passive_cycles_tests tests/weapon_passive_cycles_tests.cpp)
  target_link_libraries(weapon_passive_cycles_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_passive_cycles COMMAND weapon_passive_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_passive_cycles_original.tsv)
  set_tests_properties(original_weapon_passive_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_dual_cycles_tests tests/weapon_dual_cycles_tests.cpp)
  target_link_libraries(weapon_dual_cycles_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_dual_cycles COMMAND weapon_dual_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_dual_cycles_original.tsv)
  set_tests_properties(original_weapon_dual_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME original_weapon_registered_trigger_cycles COMMAND weapon_dual_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_registered_trigger_cycles_original.tsv registered-trigger)
  set_tests_properties(original_weapon_registered_trigger_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME original_weapon_registered_dual_cycles COMMAND weapon_dual_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_registered_dual_cycles_original.tsv registered)
  set_tests_properties(original_weapon_registered_dual_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME original_smg_dual_cycles COMMAND weapon_dual_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/smg_dual_cycles_original.tsv smg)
  set_tests_properties(original_smg_dual_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_graph_timing_tests tests/weapon_graph_timing_tests.cpp)
  target_link_libraries(weapon_graph_timing_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_graph_timing COMMAND weapon_graph_timing_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_graph_timing_original.tsv)
  set_tests_properties(original_weapon_graph_timing PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME original_weapon_charged_graph_timing COMMAND weapon_graph_timing_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_charged_graph_timing_original.tsv charged)
  set_tests_properties(original_weapon_charged_graph_timing PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME original_weapon_unequip_graph_timing COMMAND weapon_graph_timing_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_unequip_graph_timing_original.tsv put-away)
  set_tests_properties(original_weapon_unequip_graph_timing PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_reload_cycles_tests tests/weapon_reload_cycles_tests.cpp)
  target_link_libraries(weapon_reload_cycles_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_reload_cycles COMMAND weapon_reload_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_reload_cycles_original.tsv)
  set_tests_properties(original_weapon_reload_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME original_weapon_elite_reload_cycles COMMAND weapon_reload_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_elite_reload_original.tsv 3)
  set_tests_properties(original_weapon_elite_reload_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_reserve_tests tests/weapon_reserve_tests.cpp src/gameplay/weapons/weapon_magazine.cpp)
  target_include_directories(weapon_reserve_tests PRIVATE include)
  add_test(NAME original_weapon_reserve COMMAND weapon_reserve_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_reserve_original.tsv)
  set_tests_properties(original_weapon_reserve PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_animation_timing_tests tests/weapon_animation_timing_tests.cpp src/gameplay/weapons/weapon_energy.cpp)
  target_include_directories(weapon_animation_timing_tests PRIVATE include)
  add_test(NAME original_weapon_animation_timing COMMAND weapon_animation_timing_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_animation_timing_original.tsv)
  set_tests_properties(original_weapon_animation_timing PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_magazine_tests tests/weapon_magazine_tests.cpp src/gameplay/weapons/weapon_magazine.cpp)
  target_include_directories(weapon_magazine_tests PRIVATE include)
  add_test(NAME original_weapon_magazine COMMAND weapon_magazine_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_magazine_original.tsv)
  set_tests_properties(original_weapon_magazine PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(shader_spline2_tests tests/shader_spline2_tests.cpp)
  target_link_libraries(shader_spline2_tests PRIVATE halo2_cache)
  add_test(NAME original_host_shader_spline2 COMMAND shader_spline2_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shader_spline2_host_original.tsv)
  set_tests_properties(original_host_shader_spline2 PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_heat_shader_tests tests/weapon_heat_shader_tests.cpp src/gameplay/weapons/source_weapon_shader_input.cpp)
  target_include_directories(weapon_heat_shader_tests PRIVATE include)
  add_test(NAME original_weapon_heat_shader COMMAND weapon_heat_shader_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_heat_shader_original.tsv)
  set_tests_properties(original_weapon_heat_shader PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_preparation_random_tests tests/weapon_preparation_random_tests.cpp src/gameplay/weapons/weapon_energy.cpp)
  target_include_directories(weapon_preparation_random_tests PRIVATE include)
  add_test(NAME original_weapon_preparation_random COMMAND weapon_preparation_random_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_preparation_random_original.tsv)
  set_tests_properties(original_weapon_preparation_random PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_energy_tests tests/weapon_energy_tests.cpp src/gameplay/weapons/weapon_energy.cpp)
  target_include_directories(weapon_energy_tests PRIVATE include)
  add_test(NAME original_weapon_energy COMMAND weapon_energy_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_energy_original.tsv
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_vent_timing_original.tsv)
  set_tests_properties(original_weapon_energy PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_energy_cycles_tests tests/weapon_energy_cycles_tests.cpp)
  target_link_libraries(weapon_energy_cycles_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_energy_cycles COMMAND weapon_energy_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_energy_cycles_original.tsv)
  set_tests_properties(original_weapon_energy_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME original_weapon_elite_energy_cycles COMMAND weapon_energy_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_elite_energy_original.tsv 3)
  set_tests_properties(original_weapon_elite_energy_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_barrel_timing_tests tests/weapon_barrel_timing_tests.cpp src/gameplay/weapons/weapon_barrel_timing.cpp)
  target_include_directories(weapon_barrel_timing_tests PRIVATE include)
  add_test(NAME original_weapon_barrel_timing COMMAND weapon_barrel_timing_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_barrel_timing_original.tsv)
  set_tests_properties(original_weapon_barrel_timing PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_firing_cycles_tests tests/weapon_firing_cycles_tests.cpp)
  target_link_libraries(weapon_firing_cycles_tests PRIVATE halo2_systems)
  add_test(NAME original_weapon_firing_cycles COMMAND weapon_firing_cycles_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_firing_cycles_original.tsv)
  set_tests_properties(original_weapon_firing_cycles PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_effect_sound_metadata_tests tests/weapon_effect_sound_metadata_tests.cpp)
  target_link_libraries(weapon_effect_sound_metadata_tests PRIVATE halo2_systems)
  add_test(NAME weapon_effect_shared_sound_metadata COMMAND weapon_effect_sound_metadata_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
  set_tests_properties(weapon_effect_shared_sound_metadata PROPERTIES SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
    add_executable(weapon_primary_charge_tu15_tests tests/weapon_primary_charge_tu15_tests.cpp src/gameplay/weapons/source_weapon_shader_input.cpp)
    target_include_directories(weapon_primary_charge_tu15_tests PRIVATE include)
    target_link_libraries(weapon_primary_charge_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(weapon_primary_charge_tu15_tests PRIVATE -ffp-contract=off)
    add_test(NAME original_tu15_weapon_primary_charge COMMAND weapon_primary_charge_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_primary_charge_tu15_original.json")
endif()
target_sources(halo2_systems PRIVATE src/ui/hud/native_weapon_hud.cpp)
target_sources(halo2_systems PRIVATE src/campaign/campaign_source_runtime.cpp)
if(BUILD_TESTING)
  add_executable(weapon_zoom_hud_tests tests/weapon_zoom_hud_tests.cpp)
  add_executable(campaign_weapon_hud_world_tests tests/campaign_weapon_hud_world_tests.cpp)
  target_link_libraries(campaign_weapon_hud_world_tests PRIVATE halo2_systems)
  if(EXISTS "${HALO2_TU15_GAMEPLAY_XBE}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map")
    add_test(NAME campaign_weapon_hud_world COMMAND campaign_weapon_hud_world_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${HALO2_TU15_GAMEPLAY_XBE})
    set_tests_properties(campaign_weapon_hud_world PROPERTIES TIMEOUT 1200)
    foreach(campaign_hud_map IN ITEMS 04b_floodlab 05b_deltatowers)
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/${campaign_hud_map}.map")
        add_test(NAME campaign_weapon_hud_world_${campaign_hud_map} COMMAND campaign_weapon_hud_world_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/${campaign_hud_map}.map
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${HALO2_TU15_GAMEPLAY_XBE})
        set_tests_properties(campaign_weapon_hud_world_${campaign_hud_map} PROPERTIES TIMEOUT 1200)
      endif()
    endforeach()
  endif()
  target_link_libraries(weapon_zoom_hud_tests PRIVATE halo2_systems)
  add_test(NAME all_weapon_zoom_hud COMMAND weapon_zoom_hud_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
  add_executable(hud_target_anchor_tests tests/hud_target_anchor_tests.cpp)
  target_link_libraries(hud_target_anchor_tests PRIVATE halo2_cache)
  add_test(NAME original_hud_target_anchor COMMAND hud_target_anchor_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_target_anchor_original.tsv)
  set_tests_properties(original_hud_target_anchor PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(hud_target_draw_tests tests/hud_target_draw_tests.cpp)
  target_link_libraries(hud_target_draw_tests PRIVATE halo2_systems)
  add_test(NAME original_hud_target_draw COMMAND hud_target_draw_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_target_anchor_original.tsv)
  set_tests_properties(original_hud_target_draw PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(weapon_zoom_world_tests tests/weapon_zoom_world_tests.cpp)
  target_link_libraries(weapon_zoom_world_tests PRIVATE halo2_systems)
  add_executable(weapon_hud_transport_tests tests/weapon_hud_transport_tests.cpp)
  target_link_libraries(weapon_hud_transport_tests PRIVATE halo2_systems)
  add_test(NAME weapon_hud_transport COMMAND weapon_hud_transport_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
  add_executable(vehicle_weapon_hud_tests tests/vehicle_weapon_hud_tests.cpp)
  target_link_libraries(vehicle_weapon_hud_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME vehicle_weapon_hud COMMAND vehicle_weapon_hud_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vehicle_weapon_hud_original.json
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
  set_tests_properties(vehicle_weapon_hud PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle_weapon_hud_world_tests tests/vehicle_weapon_hud_world_tests.cpp)
  target_link_libraries(vehicle_weapon_hud_world_tests PRIVATE halo2_systems)
  add_executable(body_impulse_tu15_tests tests/body_impulse_tu15_tests.cpp)
  add_executable(rocket_first_person_reference_tests tests/rocket_first_person_reference_tests.cpp)
  target_link_libraries(rocket_first_person_reference_tests PRIVATE halo2_systems)
  target_link_libraries(body_impulse_tu15_tests PRIVATE halo2_systems)
  add_test(NAME original_tu15_body_impulse COMMAND body_impulse_tu15_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/body_impulse_tu15_original.json)
  if(EXISTS "${HALO2_ROCKET_GHOST_MCC_MAP}")
    get_filename_component(halo2_ghost_boost_maps "${HALO2_ROCKET_GHOST_MCC_MAP}" DIRECTORY)
    add_test(NAME original_rocket_first_person_reference COMMAND rocket_first_person_reference_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${halo2_ghost_boost_maps}"
        "${CMAKE_CURRENT_BINARY_DIR}/rocket-first-person-reference.json")
    add_test(NAME native_mcc_ghost_right_click_boost COMMAND ${CMAKE_COMMAND} -E env
        HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=ghost HALO2_TEST_GHOST_BOOST=1
        "HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${CMAKE_CURRENT_BINARY_DIR}/ghost-boost.json"
        $<TARGET_FILE:vehicle_weapon_hud_world_tests> "${HALO2_ROCKET_GHOST_MCC_MAP}"
        "${halo2_ghost_boost_maps}" native-mcc)
    set_tests_properties(native_mcc_ghost_right_click_boost PROPERTIES TIMEOUT 180)
    add_test(NAME native_mcc_rocket_lock_and_explosion COMMAND ${CMAKE_COMMAND} -E env
        HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=ghost HALO2_TEST_ROCKET_LOCK_ON=1
        "HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${CMAKE_CURRENT_BINARY_DIR}/rocket-lock.json"
        $<TARGET_FILE:vehicle_weapon_hud_world_tests> "${HALO2_ROCKET_GHOST_MCC_MAP}"
        "${halo2_ghost_boost_maps}" native-mcc)
    set_tests_properties(native_mcc_rocket_lock_and_explosion PROPERTIES TIMEOUT 180)
    foreach(rocket_target banshee scorpion wraith warthog-mg warthog-gauss)
      add_test(NAME native_mcc_rocket_lock_${rocket_target} COMMAND ${CMAKE_COMMAND} -E env
          HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=${rocket_target} HALO2_TEST_ROCKET_LOCK_ON=1
          "HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${CMAKE_CURRENT_BINARY_DIR}/rocket-lock-${rocket_target}.json"
          $<TARGET_FILE:vehicle_weapon_hud_world_tests> "${HALO2_ROCKET_GHOST_MCC_MAP}"
          "${halo2_ghost_boost_maps}" native-mcc)
      set_tests_properties(native_mcc_rocket_lock_${rocket_target} PROPERTIES TIMEOUT 180)
    endforeach()
    foreach(rocket_target warthog-mg warthog-gauss)
      add_test(NAME native_mcc_rocket_empty_${rocket_target} COMMAND ${CMAKE_COMMAND} -E env
          HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=${rocket_target} HALO2_TEST_ROCKET_LOCK_ON=1
          HALO2_TEST_ROCKET_EMPTY_TARGET=1
          "HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${CMAKE_CURRENT_BINARY_DIR}/rocket-empty-${rocket_target}.json"
          $<TARGET_FILE:vehicle_weapon_hud_world_tests> "${HALO2_ROCKET_GHOST_MCC_MAP}"
          "${halo2_ghost_boost_maps}" native-mcc)
      set_tests_properties(native_mcc_rocket_empty_${rocket_target} PROPERTIES TIMEOUT 180)
    endforeach()
  endif()
  add_executable(vehicle_weapon_hud_runtime_tests EXCLUDE_FROM_ALL tests/vehicle_weapon_hud_world_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
  target_link_libraries(vehicle_weapon_hud_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
  target_compile_definitions(vehicle_weapon_hud_runtime_tests PRIVATE HALO2_VEHICLE_HUD_GPU=1 HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
  add_dependencies(vehicle_weapon_hud_runtime_tests halo2_shaders)
  if(EXISTS "${HALO2_ROCKET_GHOST_MCC_MAP}")
    foreach(rocket_views 1 2 4)
      add_test(NAME native_mcc_rocket_flight_render_${rocket_views} COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_rocket_fov_runtime_tu15.py
          --executable $<TARGET_FILE:native_covenant_weapon_runtime_tests>
          --maps "${halo2_ghost_boost_maps}" --phase attack --visuals-only
          --players ${rocket_views} --width 1280 --height 720
          --report "${CMAKE_CURRENT_BINARY_DIR}/rocket-flight-${rocket_views}.json")
      set_tests_properties(native_mcc_rocket_flight_render_${rocket_views} PROPERTIES TIMEOUT 180)
    endforeach()
    add_test(NAME native_mcc_ghost_projectile_render COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_ghost_projectile_render_test.py
        $<TARGET_FILE:vehicle_weapon_hud_runtime_tests> "${HALO2_ROCKET_GHOST_MCC_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/ghost-projectile-render.json")
    set_tests_properties(native_mcc_ghost_projectile_render PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME native_mcc_rocket_explosion_render COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_ghost_projectile_render_test.py
        $<TARGET_FILE:vehicle_weapon_hud_runtime_tests> "${HALO2_ROCKET_GHOST_MCC_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/rocket-explosion-render.json" rocket-explosion)
    set_tests_properties(native_mcc_rocket_explosion_render PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
  endif()
  add_executable(vehicle_held_weapon_render_tests EXCLUDE_FROM_ALL tests/vehicle_held_weapon_render_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
  target_link_libraries(vehicle_held_weapon_render_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
  target_compile_definitions(vehicle_held_weapon_render_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
  add_dependencies(vehicle_held_weapon_render_tests halo2_shaders)
  foreach(stage IN ITEMS on-foot seated moving exited)
    add_test(NAME vehicle_held_weapon_${stage} COMMAND vehicle_held_weapon_render_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/coagulation.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe" 0 ${stage})
    set_tests_properties(vehicle_held_weapon_${stage} PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
  endforeach()
  add_test(NAME vehicle_held_weapon_passenger COMMAND vehicle_held_weapon_render_tests
      "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/coagulation.map"
      "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
      "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe" 1 seated)
  set_tests_properties(vehicle_held_weapon_passenger PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
  if(HALO2_MCC_GAMEPLAY_TEST_CACHE)
    foreach(stage IN ITEMS on-foot seated moving exited)
      add_test(NAME vehicle_held_weapon_mcc_${stage} COMMAND vehicle_held_weapon_render_tests
          "${HALO2_MCC_GAMEPLAY_TEST_CACHE}/halo2/h2_maps_win64_dx11/coagulation.map"
          "${HALO2_MCC_GAMEPLAY_TEST_CACHE}/halo2/h2_maps_win64_dx11" mcc 0 ${stage})
      set_tests_properties(vehicle_held_weapon_mcc_${stage} PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endforeach()
    add_test(NAME vehicle_held_weapon_mcc_passenger COMMAND vehicle_held_weapon_render_tests
        "${HALO2_MCC_GAMEPLAY_TEST_CACHE}/halo2/h2_maps_win64_dx11/coagulation.map"
        "${HALO2_MCC_GAMEPLAY_TEST_CACHE}/halo2/h2_maps_win64_dx11" mcc 1 seated)
    set_tests_properties(vehicle_held_weapon_mcc_passenger PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
  endif()
endif()
