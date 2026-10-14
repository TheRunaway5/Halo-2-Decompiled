# Same retained bank and native resource owners for the TU1.5 live world.
target_sources(halo2_systems PRIVATE src/physics_live_world_tu15.cpp src/source_runtime_cache_tags_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics_live_support_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_object_sound_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_physics_deactivation_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_combat_action_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_sandbox_special_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/source_cache_bitmap_view_tu15.cpp)
if(BUILD_TESTING)
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
    target_link_libraries(capture_source_melee_contact PRIVATE halo2_systems)
    add_executable(capture_source_weapon_cycle_tu15 tools/capture_source_weapon_cycle_tu15.cpp)
    target_link_libraries(capture_source_weapon_cycle_tu15 PRIVATE halo2_systems)
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
    set_tests_properties(original_tu15_combat_action PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_cache_bitmap_view COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/combat_cache_bitmap_view_tu15_original.json")
    set_tests_properties(original_tu15_cache_bitmap_view PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(tu15_grenade_weapon_gate_tests tests/tu15_grenade_weapon_gate_tests.cpp)
    target_link_libraries(tu15_grenade_weapon_gate_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_grenade_weapon_gate COMMAND tu15_grenade_weapon_gate_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_grenade_weapon_gate_original.json")
    set_tests_properties(original_tu15_grenade_weapon_gate PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_live_world_tu15_tests tests/physics_live_world_tu15_tests.cpp)
    target_link_libraries(physics_live_world_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_physics_live_world_tu15 COMMAND physics_live_world_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_physics_live_world_tu15 PROPERTIES TIMEOUT 120)
endif()
