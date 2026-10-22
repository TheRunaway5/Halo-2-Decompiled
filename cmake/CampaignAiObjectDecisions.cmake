if(TARGET halo2_physics_world_runtime)
    target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_campaign_ai_object_decisions_tu15_generated.cpp
        src/campaign/ai/source_campaign_ai_object_callbacks_tu15_generated.cpp src/campaign/source_campaign_birth_query_variants_tu15_generated.cpp
        src/campaign/ai/source_campaign_ai_actor_behavior_tu15_generated.cpp)
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(campaign_ai_bsp_cache_slice "${CMAKE_CURRENT_BINARY_DIR}/campaign_ai_bsp_cache_test_slice.cpp")
    add_custom_command(OUTPUT "${campaign_ai_bsp_cache_slice}"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/extract_campaign_ai_bsp_cache_test_slice_tu15.py"
            --source "${CMAKE_CURRENT_SOURCE_DIR}" --build-root "${CMAKE_CURRENT_BINARY_DIR}"
            --output "${campaign_ai_bsp_cache_slice}"
        DEPENDS tools/extract_campaign_ai_bsp_cache_test_slice_tu15.py
            src/physics/physics_live_support_tu15_generated.cpp src/audio/source_sound_listener_selection_tu15_generated.cpp
            src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp
            src/platform/native_tu15_world_memory.cpp
        VERBATIM)
    add_executable(campaign_ai_object_decisions_tu15_tests
        "${campaign_ai_bsp_cache_slice}"
        tests/campaign_ai_object_decisions_tu15_tests.cpp src/campaign/ai/source_campaign_ai_object_decisions_tu15_generated.cpp
        src/campaign/ai/source_campaign_ai_object_callbacks_tu15_generated.cpp
        src/campaign/source_campaign_birth_query_variants_tu15_generated.cpp
        src/campaign/ai/source_campaign_ai_actor_behavior_tu15_generated.cpp
        src/gameplay/units/source_damage_delivery_runtime_generated.cpp
        src/campaign/ai/source_ai_frame_tu15_generated.cpp src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp
        src/platform/native_tu15_final_startup_storage_data.cpp src/physics/native_tu15_physics_branch_data.cpp
        src/runtime/source_runtime_memory.cpp src/campaign/scripting/native_mcc_script_builtin.cpp)
    target_include_directories(campaign_ai_object_decisions_tu15_tests PRIVATE include src)
    target_link_libraries(campaign_ai_object_decisions_tu15_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    target_compile_options(campaign_ai_object_decisions_tu15_tests PRIVATE
        -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math -O0)
    target_link_options(campaign_ai_object_decisions_tu15_tests PRIVATE -Wl,--gc-sections)
    add_test(NAME original_campaign_ai_object_decisions_tu15 COMMAND campaign_ai_object_decisions_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_object_decisions_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_workspace_leases_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_object_callbacks_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_object_callback_bodies_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_callback_numeric_data_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_query_identity_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_prop_selector_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_object_tracking_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_actor_state_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_query_bucket_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_birth_query_variants_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_update_query_variants_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_remaining_query_variants_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_actor_behavior_inputs_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_script_context_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_marine_commands_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_hs_function_registry_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_shared_prop_helper_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_actor_scalars_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_bsp_audibility_cache_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_difficulty_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_camera_effect_request_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_camera_effect_request_hardware_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_damage_readonly_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_damage_reaction_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_prop_owner_lookup_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_prop_creation_admissible_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_decision_workspace_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_geometry_projection_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_child_links_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_path_distance_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_behavior_27_tu15.json")
endif()
