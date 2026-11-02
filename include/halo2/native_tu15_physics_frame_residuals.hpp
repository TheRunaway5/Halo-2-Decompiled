#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <map>
namespace halo2 {
// The caller owns ordinary writable retained stack bytes below its active root
// boundary. The step packet stays live above every residual call's argument
// frame until the complete native frame returns. This is data storage only.
struct NativeTu15PhysicsFrameScratch {std::uint32_t first{},bytes{};};
struct NativeTu15PhysicsFrameExecutionStats {
    std::uint64_t native_breakable_contact_calls{},native_breakable_damage_records{};
  std::uint64_t frame_calls{},residual_calls{};
  std::uint64_t native_dirty_island_objects_attempts{},native_dirty_island_objects_calls{},native_dirty_island_objects_refusals{},native_dirty_island_objects_kernel_entries{},native_dirty_island_objects_pending_skips{},native_dirty_island_objects_reentrant_skips{},native_dirty_island_objects_island_rows{},native_dirty_island_objects_dirty_islands{},native_dirty_island_objects_body_rows{},native_dirty_island_objects_candidate_objects{},native_dirty_island_objects_deferred_objects{},native_dirty_island_objects_component_destructions{},native_dirty_island_objects_component_frees{},native_dirty_island_objects_object_deletions{},native_dirty_island_objects_recursive_updates{},native_dirty_island_objects_flag_calls{},native_dirty_island_objects_defer_calls{},native_dirty_island_objects_state_removals{},native_dirty_island_objects_state_resets{},native_dirty_island_objects_state_refreshes{},native_dirty_island_objects_state_activations{},native_dirty_island_objects_unqualified_services{},native_dirty_island_objects_unsupported_local_sizes{};
  std::map<std::uint32_t,std::uint64_t> native_dirty_island_objects_unqualified_owners;
  std::uint64_t native_contact_pressure_attempts{},native_contact_pressure_null_worlds{},native_contact_pressure_reentrant_skips{},native_contact_pressure_islands{},native_contact_pressure_dense_islands{},native_contact_pressure_shape_queries{},native_contact_pressure_candidate_queries{},native_contact_pressure_unbound_services{},native_contact_pressure_completions{},native_contact_pressure_refusals{};
  std::uint64_t native_memory_pressure_attempts{},native_memory_pressure_level_queries{},native_memory_pressure_pool_queries{},native_memory_pressure_reentrant_skips{},native_memory_pressure_unbound_services{},native_memory_pressure_completions{},native_memory_pressure_refusals{};
  std::uint64_t native_object_publication_attempts{},native_object_publication_calls{},
    native_object_publication_refusals{},native_object_publication_root_queries{},
    native_object_publication_tree_calls{},native_object_publication_ancestor_wakes{},
    native_object_publication_marked_headers{};
  std::uint64_t world_integrations{},island_integrations{},native_force_calls{},native_pose_calls{},native_deactivation_calls{},world_residual_calls{};
  std::uint64_t native_kinematic_pose_calls{},native_fixed_pose_calls{},native_prescribed_force_calls{},native_body_policy_calls{},native_island_transition_calls{};
  std::uint64_t native_gameplay_deactivation_calls{},native_gameplay_deactivation_blocked{},
    native_gameplay_deactivation_vehicle_tests{},native_gameplay_deactivation_biped_tests{},
    native_gameplay_deactivation_body_policies{};
  std::uint64_t native_array_growth_calls{},native_array_pool_allocations{},native_array_pool_releases{};
  std::uint64_t native_array_reserve_calls{};
  std::uint64_t native_contact_features_attempts{},native_contact_features_calls{},native_contact_features_refusals{};
  std::array<std::uint64_t,2>native_contact_features_source_calls{};
  std::array<std::uint64_t,6>native_contact_features_owner_calls{};
  std::uint64_t native_simplex_helpers_attempts{},native_simplex_helpers_calls{},native_simplex_helpers_refusals{},
    native_simplex_triangle_calls{},native_closest_segment_calls{};
  std::array<std::uint64_t,5>native_simplex_helper_source_calls{};
  std::uint64_t native_vehicle_flip_handoff_attempts{},native_vehicle_flip_handoff_calls{},native_vehicle_flip_handoff_refusals{},
    native_vehicle_flip_calls{},native_vehicle_flip_flag_skips{},native_vehicle_flip_direction_skips{},
    native_vehicle_flip_timeouts{},native_vehicle_flip_orientation_skips{},native_vehicle_flip_get_calls{},
    native_vehicle_flip_set_calls{},native_vehicle_flip_completed{},native_vehicle_flip_unbound_services{},
    native_vehicle_flip_unqualified_owners{},native_vehicle_flip_residual_calls{};
  std::uint64_t native_contact_processor_calls{},native_contact_processor_contacts{},
    native_contact_processor_candidate_rows{},native_contact_processor_matched_rows{},
    native_contact_processor_updates{};
  std::uint64_t native_world_regroup_constant_info_queries{},native_world_regroup_extended_info_queries{};
  std::uint64_t native_world_regroup_attempts{},native_world_regroup_calls{},native_world_regroup_refusals{},native_world_regroup_active_visits{},native_world_regroup_flag_skips{},native_world_regroup_fully_connected{},native_world_regroup_empty_islands{},native_world_regroup_created_islands{},native_world_regroup_moved_bodies{},native_world_regroup_compacted_bodies{},native_world_regroup_moved_actions{},native_world_regroup_moved_constraints{},native_world_regroup_moved_contacts{},native_world_regroup_retained_contacts{},native_world_regroup_graph_builds{},native_world_regroup_edge_collections{},native_world_regroup_array_growths{},native_world_regroup_group_allocations{},native_world_regroup_group_releases{},native_world_regroup_temporary_constructors{},native_world_regroup_temporary_destructors{},native_world_regroup_pool_allocations{},native_world_regroup_pool_releases{},native_world_regroup_unbound_services{};
  std::uint64_t native_body_deactivation_motion_snapshot_calls{},native_body_deactivation_motion_position_queries{},native_body_deactivation_motion_disabled_bodies{},native_body_deactivation_motion_snapshot_stores{};
  std::uint64_t native_body_deactivation_attempts{},native_body_deactivation_calls{},native_body_deactivation_refusals{},native_body_deactivation_deferred_bodies{},native_body_deactivation_array_growths{},native_body_deactivation_property_queries{},native_body_deactivation_motion_publications{},native_body_deactivation_unbound_services{};
  std::uint64_t native_world_activation_agent_factories{},native_world_activation_bound_factory_attempts{},native_world_activation_bound_factory_calls{},native_world_activation_bound_factory_refusals{};
  std::uint64_t native_pair_discrete_attempts{},native_pair_discrete_calls{},native_pair_discrete_refusals{},native_pair_discrete_unbound_filters{},native_pair_discrete_unbound_motion_types{},native_pair_discrete_payload_filters{},native_pair_discrete_motion_queries{};
  std::uint64_t native_pair_island_wake_attempts{},native_pair_island_wake_calls{},native_pair_island_wake_refusals{};
  std::uint64_t native_world_merge_calls{},native_world_merge_contact_calls{},native_world_merge_body_moves{},native_world_merge_constraint_moves{},native_world_merge_disposals{},native_world_merge_refusals{},native_world_merge_unbound_services{},native_world_activation_calls{},native_world_activation_refusals{},native_world_activation_body_updates{},native_world_activation_constraint_rows{},native_world_activation_residual_calls{};
  std::uint64_t native_world_collision_calls{};
  std::uint64_t native_world_collision_listener_calls{},native_island_collision_listener_calls{};
  std::uint64_t native_world_step_listener_calls{},native_island_step_listener_calls{};
  std::uint64_t native_body_reference_calls{},native_body_reference_cohorts{},native_body_reference_moved_entries{},native_body_reference_reserves{};
  std::uint64_t native_contact_completion_calls{},native_contact_completion_contacts{},
    native_contact_completion_world_notifications{},native_contact_completion_body_notifications{},
    native_contact_completion_custom_callbacks{},native_contact_completion_null_entries_removed{};
  std::uint64_t native_camera_query_attempts{},native_camera_query_calls{},native_camera_query_refusals{},native_camera_query_resource_refusals{},native_camera_query_unknown_owners{},native_camera_query_bsp_calls{},native_camera_query_bsp_hits{},native_camera_query_cluster_visits{},native_camera_query_instanced_attempts{},native_camera_query_instanced_calls{},native_camera_query_instanced_refusals{},native_camera_query_mopp_calls{},native_camera_query_mopp_opcodes{},native_camera_query_mopp_leaves{},native_camera_query_primitive_calls{},native_camera_query_primitive_hits{},native_camera_query_dynamic_calls{},native_camera_query_dynamic_ray_calls{},native_camera_query_unprovided_reads{},native_camera_query_unprovided_writes{};
  std::uint64_t native_look_update_fp_refusals{};
  std::uint64_t native_look_update_attempts{},native_look_update_calls{},native_look_update_refusals{},native_look_update_context_calls{},native_look_update_context_refusals{},native_look_update_camera_calls{},native_look_update_camera_completions{},native_look_update_camera_onfoot_calls{},native_look_update_camera_marker_calls{},native_look_update_marker_calls{},native_look_update_angle_calls{},native_look_update_angle_completions{},native_look_update_angle_refusals{},native_look_update_query_refusals{},native_look_update_marker_observer_refusals{};
  std::uint64_t native_look_reorientation_attempts{},native_look_reorientation_calls{},native_look_reorientation_refusals{},native_look_reorientation_kernel_calls{},native_look_reorientation_kernel_refusals{},native_look_reorientation_yaw_wraps{};
  std::uint64_t native_look_initialization_attempts{},native_look_initialization_calls{},native_look_initialization_refusals{},native_look_initialization_cleared_records{},native_look_initialization_unit_directions{},native_look_initialization_unchanged{};
  std::uint64_t native_contact_body_attempts{},native_contact_body_calls{},native_contact_body_refusals{},native_contact_body_point_queries{},native_contact_body_property_queries{},native_contact_body_motion_type_queries{},native_contact_body_point_velocity_queries{},native_contact_body_impulse_requests{},native_contact_body_wake_attempts{},native_contact_body_wake_calls{},native_contact_body_wake_refusals{},native_contact_body_unqualified_owners{},native_contact_body_unsupported_fp_profiles{};
  std::map<std::uint32_t,std::uint64_t> native_contact_body_refused_owners;
  std::uint64_t native_main_heap_release_attempts{},native_main_heap_release_calls{},native_main_heap_release_refusals{},native_main_heap_release_wrapper_calls{},native_main_heap_release_ledger_calls{},native_main_heap_release_visited{},native_main_heap_release_matches{},native_main_heap_release_moved_rows{},native_main_heap_release_kernel_completed{},native_main_heap_release_unbound_services{};
  std::map<std::uint32_t,std::uint64_t> native_main_heap_release_refused_owners;
  std::uint64_t native_contact_entity_calls{},native_contact_entity_missing_body_pairs{},
    native_contact_entity_property_queries{},native_contact_entity_disabled_components{},
    native_contact_entity_dispatch_calls{};
  std::uint64_t native_deferred_operation_calls{},native_deferred_action_calls{},native_deferred_delete_calls{},native_deferred_update_calls{},native_deferred_merges{},native_deferred_body_removals{},native_deferred_update_removals{},native_deferred_reference_releases{},native_deferred_destructions{};
  std::uint64_t native_constraint_calls{},native_dynamic_accumulator_calls{},native_constraint_setup_calls{};
  std::uint64_t native_island_collision_calls{};
  std::uint64_t native_island_agent_flag_clear_calls{},native_island_agent_flag_set_calls{},
    native_island_agent_process_calls{},native_island_agent_finish_calls{},
    native_island_contact_minimum_calls{},native_island_contact_time_calls{},
    native_island_contact_buffers_constructed{},native_island_contact_buffers_destroyed{};
  std::uint64_t native_cached_motion_calls{},native_cached_motion_rewinds{},native_cached_motion_advances{},
    native_contact_fixed_motion_calls{},contact_motion_residual_calls{};
  std::uint64_t native_contact_advance_calls{},native_contact_advance_motion_calls{},
    native_contact_advance_process_calls{},native_contact_advance_single_iterations{},
    native_contact_advance_paired_iterations{},native_contact_advance_negative_single_restores{},
    native_contact_advance_negative_paired_restores{};
  std::uint64_t native_motion_commit_calls{};
  std::array<std::uint64_t,3> native_motion_commit_kinds{};
  std::uint64_t native_broad_phase_calls{};
  std::uint64_t native_broad_phase_edge_attempts{},native_broad_phase_edge_calls{},native_broad_phase_edge_refusals{},native_broad_phase_edge_objects{},native_broad_phase_edge_shifted_edges{},native_broad_phase_edge_created_pairs{},native_broad_phase_edge_removed_pairs{},native_broad_phase_edge_special_additions{},native_broad_phase_edge_special_removals{},native_broad_phase_edge_array_growths{};

  std::uint64_t native_phantom_object_spatial_flag_queries{},native_phantom_object_spatial_packets{},native_phantom_object_spatial_chain_updates{},native_phantom_object_spatial_chain_rows{},native_phantom_object_spatial_record_searches{},native_phantom_object_spatial_searched_rows{},native_phantom_object_spatial_matched_rows{},native_phantom_object_spatial_missing_rows{},native_phantom_object_spatial_word_copies{},native_phantom_object_spatial_byte_copies{},native_phantom_object_spatial_zero_copies{};
  std::uint64_t native_phantom_object_deletion_attempts{},native_phantom_object_deletion_calls{},native_phantom_object_deletion_refusals{},native_phantom_object_deletion_unbound_services{},native_phantom_object_detachment_calls{},native_phantom_object_detachment_objects{},native_phantom_object_detachment_refusals{},native_phantom_object_spatial_queries{},native_phantom_object_tracking_clears{},native_phantom_object_player_clears{},native_phantom_object_schedule_updates{},native_phantom_object_reference_clears{};
  std::uint64_t native_phantom_added_listener_attempts{},native_phantom_added_listener_calls{},native_phantom_added_listener_refusals{},native_phantom_added_components_marked{},native_phantom_added_protection_queries{},native_phantom_added_objects_protected{},native_phantom_added_delete_requests{},native_phantom_added_unqualified_deletions{};
  std::uint64_t native_phantom_pair_create_notifications{},native_phantom_pair_create_unqualified_owners{},native_phantom_pair_create_simple_adds{},native_phantom_pair_create_complex_adds{};
  std::uint64_t native_pair_consumer_attempts{},native_pair_consumer_calls{},
    native_pair_consumer_refusals{},native_pair_consumer_nonempty_calls{},
    native_pair_consumer_filter_calls{},native_pair_consumer_create_calls{},
    native_pair_consumer_delete_calls{},native_pair_consumer_unqualified_services{};
  std::uint64_t native_pair_consumer_owned_allocations{},native_pair_consumer_owned_releases{},
    native_pair_consumer_arena_allocations{},native_pair_consumer_arena_releases{},
    native_pair_consumer_pool_allocations{},native_pair_consumer_pool_releases{};
  std::uint64_t native_pair_consumer_phantom_delete_calls{},native_pair_consumer_phantom_notifications{},
    native_pair_consumer_phantom_noops{},native_pair_consumer_phantom_unqualified_owners{};
  std::uint64_t native_pair_consumer_phantom_overlap_remove_calls{},native_pair_consumer_phantom_overlap_listener_noops{};
  std::uint64_t native_pair_consumer_simple_phantom_overlap_remove_calls{};
  std::uint64_t native_stack_arena_allocations{},native_stack_arena_releases{},native_stack_arena_refusals{};
  std::uint64_t native_joint_builder_attempts{},native_joint_builder_calls{},native_joint_builder_refusals{},native_joint_builder_unsupported_fp{},native_joint_builder_kernel_entries{},native_joint_builder_kernel_completed{},native_joint_builder_headers{},native_joint_builder_angular_rows{},native_joint_builder_motors{},native_joint_builder_linear_requests{},native_joint_builder_linear_calls{},native_joint_builder_linear_completed{},native_joint_builder_linear_rows{},native_joint_builder_linear_refusals{},native_joint_builder_limits{},native_joint_builder_positive_unwraps{},native_joint_builder_negative_unwraps{},native_joint_builder_unbound_linear{},native_joint_builder_linear_unaligned{},native_joint_builder_linear_unsupported_fp{};
  std::uint64_t native_constraint_arena_allocations{},native_constraint_arena_releases{},native_constraint_arena_refusals{};
  std::uint64_t native_contact_agent_cleanup_calls{},native_contact_agent_record_cleanups{},native_contact_agent_pointer_cleanups{},native_contact_reference_disposals{},native_contact_agent_child_residuals{},native_contact_agent_destructor_residuals{},native_contact_agent_cleanup_refusals{};
  std::uint64_t native_reversed_contact_process_attempts{},native_reversed_contact_process_completions{},native_reversed_contact_process_refusals{},native_reversed_wrapped_contact_process_calls{},native_reversed_compound_contact_process_calls{},native_reversed_contact_process_child_returns{},native_reversed_contact_normal_vectors{};
  std::uint64_t native_contact_process_calls{},native_contact_process_completions{},native_contact_process_refusals{},native_wrapped_contact_process_calls{},native_compound_contact_process_calls{},native_contact_process_geometry_calls{},native_contact_process_child_calls{},native_contact_process_child_residuals{};
  std::uint64_t native_wrapped_factory_attempts{},native_wrapped_factory_calls{},native_wrapped_factory_refusals{},native_wrapped_factory_wrapped_entries{},native_wrapped_factory_transformed_entries{},native_wrapped_factory_convex_entries{},native_wrapped_factory_class_queries{},native_wrapped_factory_unqualified_owners{};
  std::uint64_t native_list_agent_factory_attempts{},native_list_agent_factory_calls{},native_list_agent_factory_refusals{},native_list_agent_count_queries{},native_list_agent_filter_queries{},native_list_agent_child_factories{},native_list_agent_child_returns{};
  std::uint64_t native_transformed_agent_factory_attempts{},native_transformed_agent_factory_calls{},native_transformed_agent_factory_refusals{};
  std::uint64_t native_contact_point_constraint_reference_calls{},native_contact_point_constraint_reference_decrements{},native_contact_point_constraint_reference_runtime_removals{},native_contact_point_constraint_reference_runtime_refusals{};
  std::uint64_t native_contact_point_part_calls{},native_contact_point_part_completions{},native_contact_point_part_refusals{},native_contact_point_part_inactive{},native_contact_point_part_material_queries{},native_contact_point_part_response_vectors{},native_contact_point_part_zero_properties{},native_contact_point_part_vector_properties{},native_contact_point_part_conveyor_bindings{},native_contact_point_part_function_calls{},native_contact_point_part_function_refusals{},native_contact_point_part_function_fp_refusals{};
  std::uint64_t native_contact_point_attempts{},native_contact_point_calls{},native_contact_point_refusals{},native_contact_point_profile_refusals{},native_contact_point_unqualified_services{},native_contact_point_noop_listeners{},native_contact_point_gameplay_listeners{},native_contact_point_gameplay_rows{},native_contact_point_metadata_calls{},native_contact_point_world_updates{},native_contact_point_world_queued_updates{},native_contact_point_island_detaches{},native_contact_point_info_queries{},native_contact_point_reference_releases{},native_contact_point_effect_requests{},native_contact_point_part_requests{};
  std::uint64_t native_contact_agent_cached_cleanups{},native_contact_agent_wrapped_cleanups{},native_contact_agent_native_destructors{},native_contact_agent_point_residuals{};
  std::uint64_t native_contact_manager_base_lock_releases{};
  std::uint64_t native_contact_manager_deferred_growths{};
  std::uint64_t native_contact_manager_cleanups{},native_contact_collector_cleanups{},native_contact_listener_unregistrations{},native_contact_listener_deferred_references{},native_contact_manager_service_residuals{},native_contact_manager_cleanup_refusals{};
  std::uint64_t native_constraint_effect_selection_calls{},native_constraint_effect_selected_rows{},native_constraint_effect_camera_tests{},native_constraint_effect_distance_refusals{},native_constraint_effect_unknown_constants{};
  std::uint64_t native_gsk_agent_attempts{},native_gsk_agent_calls{},native_gsk_agent_refusals{};
  std::uint64_t native_pair_creation_attempts{},native_pair_creation_calls{},native_pair_creation_refusals{},native_convex_agent_attempts{},native_convex_agent_calls{},native_convex_agent_refusals{},native_convex_agent_pool_allocations{},native_convex_agent_pool_releases{},native_convex_agent_class_queries{},native_convex_agent_centers{},native_convex_agent_supports{},native_convex_agent_encodings{};
  std::uint64_t native_pair_manager_factory_attempts{},native_pair_manager_factory_calls{},native_pair_manager_factory_refusals{},native_pair_manager_factory_unqualified_factories{},native_pair_manager_factory_body_destructor_refusals{},native_pair_manager_factory_base_lock_releases{},native_pair_manager_factory_base_lock_refusals{},native_pair_manager_factory_source_factories{},native_pair_manager_factory_constructors{},native_pair_manager_factory_collectors{},native_pair_manager_factory_registrations{},native_pair_manager_factory_unregistrations{},native_pair_manager_factory_deferred_references{},native_pair_manager_factory_array_growths{},native_pair_manager_factory_body_destructions{},native_pair_manager_factory_protected_references{},native_pair_manager_factory_storage_releases{},native_pair_manager_factory_collector_destructions{},native_pair_manager_factory_collector_base_lock_releases{};
  std::uint64_t native_pair_merge_queue_attempts{},native_pair_merge_queue_calls{},native_pair_merge_queue_refusals{},native_pair_defer_queue_attempts{},native_pair_defer_queue_calls{},native_pair_defer_queue_refusals{};
  std::uint64_t native_inactive_island_cleanup_calls{},native_inactive_island_agent_invalidations{},
    native_inactive_island_manager_invalidations{},native_inactive_island_cleared_pairs{},
    native_inactive_island_policy_queries{},native_inactive_island_policy_selections{},
    native_inactive_island_native_policy_calls{},native_inactive_island_policy_residuals{},
    native_inactive_island_cleanup_refusals{};
  std::uint64_t native_shape_bounds_box_calls{},native_shape_bounds_convex_calls{},
    native_shape_bounds_transformed_calls{},native_shape_bounds_forward_calls{},native_shape_bounds_motion_calls{},
    native_shape_bounds_centered_box_calls{},native_shape_bounds_centered_radius_calls{},shape_bounds_residual_calls{};
  std::uint64_t native_shape_bounds_endpoint_calls{},native_shape_bounds_compound_calls{},
    native_shape_bounds_compound_child_calls{};
  std::uint64_t native_shape_bounds_sphere_calls{};
  std::uint64_t native_constraint_producer_axis_calls{},native_constraint_producer_scaled_calls{},
    native_constraint_producer_base_calls{},native_constraint_producer_base_rows{},
    native_constraint_producer_property_lookups{};
  std::uint64_t native_constraint_direct_base_calls{},native_constraint_direct_base_rows{};
  std::array<std::uint64_t,3> native_constraint_producer_emissions{},
    native_constraint_producer_completed_emissions{};
  // Diagnostic counts of actual named residual attempts. Native kernels do
  // not execute these identities; the explicit migration service owns them.
  std::map<std::uint32_t,std::uint64_t> native_reversed_list_agent_destructor_refused_owners;
  std::uint64_t native_reversed_list_agent_destructor_attempts{};
  std::uint64_t native_reversed_list_agent_destructor_calls{};
  std::uint64_t native_reversed_list_agent_destructor_refusals{};
  std::uint64_t native_reversed_list_agent_destructor_unqualified_owners{};
  std::map<std::uint32_t,std::uint64_t> native_pair_deletion_refused_owners;
  std::uint64_t native_pair_deletion_attempts{};
  std::uint64_t native_pair_deletion_calls{};
  std::uint64_t native_pair_deletion_refusals{};
  std::uint64_t native_pair_deletion_agent_cleanups{};
  std::uint64_t native_pair_deletion_manager_releases{};
  std::uint64_t native_pair_deletion_unqualified_owners{};
  std::uint64_t native_pair_deletion_nested_refusals{};
  std::map<std::uint32_t,std::uint64_t> residual_owner_calls,shape_bounds_residual_owners;
  std::uint64_t native_solver_custom_calls{};
  std::uint64_t native_solver_clock_calls{};
  std::array<std::uint64_t,3> native_solver_custom_kinds{};
  std::uint64_t native_solver_calls{},native_solver_initializations{},native_solver_iterations{},native_solver_primary_rows{},native_solver_secondary_rows{},native_solver_exported_rows{},native_solver_custom_refusals{};
  std::array<std::uint64_t,4> native_lift_scalar_calls{};
  std::uint64_t native_lift_power_calls{},native_lift_errno_writes{},native_lift_observation_refusals{};
  std::uint64_t native_vehicle_rate_acceleration_calls{},native_vehicle_rate_approach_calls{},
    native_vehicle_rate_cursor_calls{},native_vehicle_rate_refusals{};
  std::uint64_t native_object_velocity_handoff_attempts{},native_object_velocity_handoff_calls{},
    native_object_velocity_handoff_refusals{},native_object_velocity_calls{},
    native_object_velocity_refusals{},native_object_velocity_unqualified_owners{},
    native_object_velocity_body_wakes{},native_object_velocity_island_wakes{},
    native_object_velocity_agent_factories{},native_object_velocity_pool_allocations{},
    native_object_velocity_unconverted_factories{},native_object_velocity_unconverted_getters{},
    native_object_velocity_residual_calls{};
  std::uint64_t native_object_velocity_payload_predicates{},
    native_object_velocity_manager_factories{},
    native_object_velocity_manager_constructors{},
    native_object_velocity_manager_registrations{},
    native_object_velocity_unconverted_managers{},
    native_object_velocity_policy_calls{},
    native_object_velocity_policy_constructors{},
    native_object_velocity_policy_bounds{},
    native_object_velocity_unconverted_policy_types{},
    native_object_velocity_unconverted_policy_destructions{};
  std::map<std::uint32_t,std::uint64_t> native_object_velocity_policy_bounds_owners,
    native_object_velocity_unconverted_bounds_owners;
  std::uint64_t native_object_velocity_constraint_updates{},
    native_object_velocity_constraint_flag_skips{},
    native_object_velocity_constraint_empty_banks{},
    native_object_velocity_constraint_rows{},
    native_object_velocity_constraint_unbound_services{},
    native_object_velocity_constraint_lookup_calls{},
    native_object_velocity_constraint_capacity_calls{},
    native_object_velocity_constraint_unbound_removals{};
  std::uint64_t native_constraint_removal_calls{},native_constraint_instance_deletions{},
    native_constraint_list_removals{},native_constraint_local_releases{},
    native_constraint_global_releases{},native_constraint_unbound_handle_releases{},
    native_constraint_unbound_runtime_removals{};
  std::uint64_t native_constraint_creation_calls{},native_constraint_created{},
    native_constraint_creation_allocations{},native_constraint_normalize_calls{},
    native_constraint_local_point_calls{},native_constraint_creation_storage_refusals{},
    native_constraint_creation_numerical_refusals{},native_constraint_handle_calls{},
    native_constraint_valid_handles{},native_constraint_retained_handle_marks{},
    native_constraint_handle_unbound_services{};
  std::uint64_t native_constraint_handle_destruction_calls{},native_constraint_handle_row_deletions{},
    native_constraint_event_deletions{},native_constraint_object_detaches{},
    native_constraint_child_unbound_services{},native_constraint_tag_value_calls{},
    native_constraint_tag_value_publications{};
  std::uint64_t native_gameplay_policy_destructions{};
  std::uint64_t native_constraint_runtime_calls{},native_constraint_runtime_counter_updates{},
    native_constraint_runtime_state_updates{},native_constraint_runtime_geometry_updates{},
    native_constraint_runtime_effect_selections{},native_constraint_runtime_effect_publications{},
    native_constraint_runtime_handle_creations{},native_constraint_runtime_storage_refusals{},
    native_constraint_runtime_profile_refusals{},native_constraint_runtime_unbound_services{};
  std::map<std::uint32_t,std::uint64_t> native_object_velocity_residual_owners;
  std::uint64_t native_schema_emitter_calls{},native_schema_emitter_completed_calls{},
    native_schema_emitter_cursor_calls{},native_schema_emitter_refusals{};
  std::array<std::uint64_t,3> native_schema_emitter_caller_kinds{};
  std::array<std::uint64_t,5> native_schema_emitter_refusal_kinds{};
};
using NativeTu15PhysicsFrameResidualCall=std::function<void(std::uint32_t,
  SourcePhysicsContactRuntimeAbi&,const SourcePhysicsContactRecordObserver&)>;
using NativeTu15PhysicsFrameWorldStep=std::function<void(NativeTu15PhysicsWorld,
  const NativeTu15PhysicsStep&,NativeTu15PhysicsFrameFp&)>;
// Migration-only binding. The caller supplies its actual standalone call state,
// including live x87 values/MMX aliases and FS base. Returned residual FP/live
// values survive into the next dependency. No root EAX/flags/XMM output is
// fabricated, and no lifted1CC8D0 root is invoked. Nonzero residual_calls mean
// this execution has NOT qualified a complete native-only physics closure.
void native_tu15_step_physics_frame_with_residuals(SourcePhysicsContactMemory&,
  NativeTu15PhysicsFrameFp&,SourcePhysicsContactRuntimeAbi& actual_residual_state,
  NativeTu15PhysicsFrameScratch,const NativeTu15PhysicsFrameResidualCall&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15PhysicsFrameExecutionStats* = nullptr,
  const NativeTu15PhysicsFrameWorldStep& = {});
} // namespace halo2
