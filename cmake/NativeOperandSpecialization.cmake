include_guard(GLOBAL)
option(HALO2_SPECIALIZE_NATIVE_OPERANDS "Fold authenticated literal native executable operands" ON)

function(halo2_native_operand_source output source)
    get_filename_component(original "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(NOT HALO2_SPECIALIZE_NATIVE_OPERANDS)
        set(${output} "${original}" PARENT_SCOPE)
        return()
    endif()
    get_filename_component(stem "${source}" NAME_WE)
    file(RELATIVE_PATH relative "${CMAKE_CURRENT_SOURCE_DIR}/src" "${original}")
    set(specialized "${CMAKE_CURRENT_BINARY_DIR}/native-operands/${relative}")
    string(REGEX REPLACE "\\.cpp$" ".operands.json" receipt "${specialized}")
    set(binding_helpers)
    # Standalone miniature projects may stage only the specializer. Real
    # aliases must be regenerated when an installed sibling's policy changes.
    foreach(helper native_page_window_replacements.py native_page_retire_replacements.py native_page_allocate_replacements.py native_stack_probe_replacements.py native_halfword_bit_query_replacements.py native_resource_busy_replacements.py native_mopp_aabb_replacements.py native_mopp_aabb_canonical_replacements.py native_polygon_support_replacements.py native_polygon_features_replacements.py native_polygon_prepared_owners.py native_simplex_triangle_replacements.py native_simplex_triangle_scene_replacements.py native_simplex_face_refine_replacements.py native_simplex_face_select_replacements.py native_closest_segments_replacements.py native_simplex_paired_replacements.py native_simplex_boundary_replacements.py native_model_marker_lookup_replacements.py native_object_node_rebuild_replacements.py native_solver_hull_append_replacements.py native_zero_shape_support_replacements.py native_box_shape_support_replacements.py native_packed_rotation_replacements.py native_packed_rotation_source_inputs.py native_codec_translation_replacements.py native_codec_scale_replacements.py native_codec_rotation_interpolation_replacements.py native_codec_translation_interpolation_replacements.py native_codec_scale_interpolation_replacements.py native_codec_copy_replacements.py native_object_type_query_replacements.py native_object_body_replacements.py native_object_material_lookup_replacements.py native_object_lookup_copies_replacements.py native_object_lookup_canonical_replacements.py native_solver_hull_support_pair_replacements.py native_solver_hull_edge_replacements.py native_solver_hull_initialize_replacements.py native_solver_hull_line_replacements.py native_solver_hull_sample_replacements.py native_solver_hull_minimum_replacements.py native_solver_hull_clear_replacements.py native_solver_hull_clip_replacements.py native_arena_world_initialize_replacements.py native_arena_initialize_replacements.py native_arena_initialize_continuation_replacements.py native_arena_owner_policy_prototype.py native_arena_world_policy_prototype.py native_arena_world_producer_inputs.py native_projectile_function_query_replacements.py native_object_definition_mode_query_replacements.py native_object_table_ratio_replacements.py native_map_world_property_query_replacements.py native_device_property_query_replacements.py native_node_progress_query_replacements.py native_node_function_progress_replacements.py native_object_function_query_replacements.py native_object_function_scalar_replacements.py native_vehicle_render_root_replacements.py native_flight_property_query_replacements.py native_object_record_ratio_replacements.py native_object_ratio_caller_replacements.py native_indexed_datum_gate_replacements.py native_particle_type_filter_replacements.py native_particle_envelope_replacements.py native_particle_record_replacements.py native_death_cycle_flight_query_replacements.py native_contact_predicate_replacements.py native_vehicle_effect_ratio_replacements.py native_selected_operand_binding.py native_pvs_flood_replacements.py native_object_sphere_query_replacements.py native_object_tick_replacements.py native_object_subtree_replacements.py native_cluster_distance_replacements.py native_collision_ray_replacements.py native_device_update_replacements.py native_campaign_ai_workspace_replacements.py native_live_support_leaves_replacements.py native_object_hierarchy_root_replacements.py native_frame_misc_replacements.py native_effect_owner_replacements.py native_world_leaves_replacements.py native_contact_array_replacements.py native_local_buffers_replacements.py native_collection_agent_replacements.py native_manifold_replacements.py native_key_sort_replacements.py native_shape_key_filter_replacements.py native_convex_pair_replacements.py native_phantom_island_replacements.py native_bsp_child_shape_replacements.py native_structure_aabb_query_replacements.py native_child_agents_replacements.py native_object_flag_update_replacements.py native_body_motion_snapshot_replacements.py
        native_particle_observer_lod_replacements.py native_particle_location_emitters_replacements.py native_particle_location_entry_replacements.py native_particle_birth_prefix_replacements.py native_particle_birth_placement_replacements.py native_particle_birth_interface_replacements.py native_emitter_transform_replacements.py native_emitter_emission_replacements.py native_emitter_orientation_replacements.py native_collection_filter_world_replacements.py native_soa_support_world_replacements.py native_swept_support_world_replacements.py
        native_particle_system_update_replacements.py native_particle_system_attachment_replacements.py native_particle_system_selection_replacements.py native_particle_system_contact_basis_replacements.py native_particle_system_velocity_basis_replacements.py native_particle_drain_counts_replacements.py native_particle_contact_walk_replacements.py native_particle_contact_flags_replacements.py native_particle_contact_query_replacements.py native_particle_contact_response_replacements.py native_particle_contact_actions_replacements.py native_particle_location_update_replacements.py
        native_particle_location_prepare_replacements.py
        native_particle_velocity_properties_replacements.py
        native_particle_velocity_direction_replacements.py
        native_particle_velocity_angles_replacements.py
        native_particle_velocity_remaining_replacements.py
        native_damage_update_entry_replacements.py native_damage_update_return_replacements.py native_damage_update_sections_replacements.py
        native_damage_update_notification_replacements.py native_damage_update_vitality_gate_replacements.py
        native_damage_descriptor_refresh_replacements.py
        native_damage_due_response_replacements.py native_damage_response_amount_replacements.py
        native_damage_response_secondary_replacements.py
        native_contact_cache_update_30cd00_replacements.py
        native_sphere_center_bounds_replacements.py
        native_two_bone_solve_replacements.py
        native_polygon_intersection_replacements.py
        native_havok_plane_314a30_replacements.py
        native_weapon_animation_selection_replacements.py
        native_bitmap_residency_replacements.py
        native_physical_result_contact_e4dc0_replacements.py
        native_physical_result_decay_e51d6_replacements.py
        native_physical_result_classification_e4b81_replacements.py
        native_physical_result_final_branches_replacements.py
        native_physical_result_activity_replacements.py
        native_pose_setup_c85a0_replacements.py
        native_pose_angle_c8930_replacements.py
        native_pose_completion_c8b65_replacements.py
        native_pose_normalization_c8827_replacements.py
        native_pose_first_axis_c8704_replacements.py
        native_pose_second_axis_c89e3_replacements.py
        native_animation_request_replacements.py
        native_world_position_transform_2e4690_replacements.py
        native_world_pressure_2335b0_replacements.py
        native_vehicle_x87_dispatch_331ecb_replacements.py
        native_vehicle_wheel_offset_20e414_replacements.py
        native_vehicle_wheel_cross_product_20e481_replacements.py
        native_vehicle_wheel_force_20e4f4_replacements.py
        native_vehicle_wheel_response_20e6c3_replacements.py
        native_vehicle_wheel_output_20e91f_replacements.py
        native_vehicle_scratch_reset_20ea32_replacements.py
        native_vehicle_wheel_curve_20e84a_replacements.py
        native_vehicle_contact_output_20f6e0_replacements.py
        native_vehicle_contact_projection_20ee92_replacements.py
        native_vehicle_contact_query_20f3a8_replacements.py
        native_vehicle_contact_collection_20ea64_replacements.py
        native_vehicle_marker_emission_1e1200_replacements.py
        native_vehicle_marker_matrix_bad60_replacements.py
        native_damage_update_flag_route_replacements.py native_damage_update_amounts_replacements.py
        native_damage_update_shield_recovery_replacements.py
        native_mopp_ray_traversal_replacements.py native_particle_draw_entry_replacements.py
        native_particle_physics_shapes_replacements.py
        native_particle_transform_basis_replacements.py
        native_particle_transform_projection_replacements.py
        native_particle_transform_matrix_calls_replacements.py
        native_particle_emitter_bound_replacements.py
        native_particle_update_location_transforms_replacements.py
        native_particle_cleanup_251650_replacements.py
        native_effect_observer_retire_17e250_replacements.py
        native_weapon_hud_status_1055f0_replacements.py
        native_weapon_hud_status_reload_1055f0_replacements.py
        native_lens_query_walk_replacements.py
        native_death_cycle_opening_replacements.py
        native_death_cycle_activity_replacements.py
        native_death_cycle_transition_replacements.py
        native_death_cycle_expiration_replacements.py
        native_death_cycle_motion_replacements.py
        native_death_cycle_motion_records_10387c_replacements.py
        native_death_cycle_timer_tail_replacements.py
        native_death_cycle_object_timer_replacements.py
        native_death_cycle_timer_notification_replacements.py
        native_death_cycle_record_route_replacements.py
        native_death_cycle_record_blend_replacements.py
        native_death_cycle_motion_finalize_replacements.py
        native_death_cycle_motion_cleanup_replacements.py
        native_death_cycle_motion_link_replacements.py
        native_death_cycle_record_effects_replacements.py
        native_player_finished_1f0670_replacements.py
        native_player_finished_angle_slot_replacements.py
        native_player_finished_weapon_slot_replacements.py
        native_player_finished_hot_slots_replacements.py
        native_player_finished_cold_slots_replacements.py
        native_particle_observer_lens_replacements.py
        native_collision_pair_2f88d0_replacements.py
        native_convex_process_prefix_2f8b60_replacements.py
        native_collision_cache_309060_replacements.py
        native_collision_cache_305560_contact_replacements.py
        native_contrail_vertex_emission_replacements.py
        native_contrail_point_sample_replacements.py
        native_contrail_point_update_replacements.py
        native_contrail_camera_facing_replacements.py
        native_contrail_adjacent_sample_replacements.py
        native_contrail_object_tint_replacements.py
        native_contrail_object_color_replacements.py
        native_contrail_orientation_setup_replacements.py
        native_contrail_draw_frame_replacements.py
        native_contrail_planar_facing_replacements.py
        native_contrail_bitmap_selector_replacements.py
        native_contrail_effect_launch_replacements.py
        native_contrail_effect_projection_replacements.py
        native_contrail_effect_tagged_lookup_replacements.py
        native_contrail_effect_default_vector_replacements.py
        native_contrail_effect_authored_vector_replacements.py
        native_contrail_effect_authored_offset_replacements.py
        native_contrail_view_fade_replacements.py
        native_contrail_transition_curve_replacements.py
        native_fog_resolution_replacements.py native_fog_accumulation_replacements.py
        native_fog_projected_merge_replacements.py
        native_fog_cluster_sky_replacements.py
        native_fog_remaining_replacements.py
        native_patchy_fog_replacements.py native_fog_cluster_finish_replacements.py
        native_fog_plane_classifier_replacements.py native_fog_plane_projection_replacements.py
        native_fog_plane_output_replacements.py native_fog_plane_transform_replacements.py
        native_fog_plane_call_replacements.py
        native_mopp_quantized_aabb_30a1e0_replacements.py)
        set(helper_path "${CMAKE_CURRENT_SOURCE_DIR}/tools/${helper}")
        if(EXISTS "${helper_path}")
            list(APPEND binding_helpers "${helper_path}")
        endif()
    endforeach()
    add_custom_command(OUTPUT "${specialized}" "${receipt}"
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/specialize_native_operands.py"
            --source "${original}" --output "${specialized}"
        DEPENDS "${original}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/specialize_native_operands.py" ${binding_helpers}
        COMMENT "Specializing authenticated literal operands from ${stem}" VERBATIM)
    # Retain any source-specific build contracts; the containing target keeps
    # its original libraries, definitions and strict arithmetic flags.
    foreach(property COMPILE_OPTIONS COMPILE_FLAGS COMPILE_DEFINITIONS INCLUDE_DIRECTORIES)
        get_source_file_property(value "${source}" ${property})
        if(value AND NOT value STREQUAL "NOTFOUND")
            set_source_files_properties("${specialized}" PROPERTIES ${property} "${value}")
        endif()
    endforeach()
    get_filename_component(original_directory "${original}" DIRECTORY)
    set_property(SOURCE "${specialized}" APPEND PROPERTY INCLUDE_DIRECTORIES "${original_directory}")
    set(${output} "${specialized}" PARENT_SCOPE)
endfunction()

function(halo2_specialize_native_target target)
    if(NOT HALO2_SPECIALIZE_NATIVE_OPERANDS OR NOT TARGET ${target})
        return()
    endif()
    get_target_property(sources ${target} SOURCES)
    set(specialized_directory "${CMAKE_CURRENT_BINARY_DIR}/native-operands")
    set(updated)
    foreach(source IN LISTS sources)
        get_filename_component(original "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        # Explicitly specialized sources are already target members. They
        # become readable after the first build and keep their dispatch map.
        cmake_path(IS_PREFIX specialized_directory "${original}" NORMALIZE already_specialized)
        if(NOT already_specialized AND EXISTS "${original}" AND source MATCHES "_generated\\.cpp$")
            file(STRINGS "${original}" mapping LIMIT_COUNT 1
                REGEX "^std::uint32_t selected\\(std::uint32_t p,SourcePhysicsContactImage image\\)")
            if(mapping)
                halo2_native_operand_source(specialized "${source}")
                list(APPEND updated "${specialized}")
                continue()
            endif()
        endif()
        list(APPEND updated "${source}")
    endforeach()
    set_property(TARGET ${target} PROPERTY SOURCES "${updated}")
endfunction()
