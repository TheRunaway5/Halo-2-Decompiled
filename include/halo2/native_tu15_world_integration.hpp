#pragma once
#include "halo2/native_tu15_gameplay_deactivation_policy.hpp"
#include "halo2/native_tu15_contact_entity_processor.hpp"
#include "halo2/native_tu15_island_integration.hpp"
#include "halo2/native_tu15_island_agent_phase.hpp"
#include "halo2/native_tu15_island_contact_advance.hpp"
#include "halo2/native_tu15_cached_motion.hpp"
#include "halo2/native_tu15_world_deferred.hpp"
#include "halo2/native_tu15_body_references.hpp"
#include "halo2/native_tu15_contact_completion.hpp"
#include "halo2/native_tu15_contact_entity_notification.hpp"
#include "halo2/native_tu15_sphere_shape_bounds.hpp"
#include "halo2/native_tu15_compound_shape_bounds.hpp"
#include "halo2/native_tu15_centered_shape_bounds.hpp"
#include "halo2/native_tu15_inactive_island_cleanup.hpp"
#include "halo2/native_tu15_body_policy.hpp"
#include "halo2/native_tu15_island_contact_merge.hpp"
#include "halo2/native_tu15_object_activation_residuals.hpp"
#include "halo2/native_tu15_contact_process_binding.hpp"
#include "halo2/native_tu15_island_regroup_binding.hpp"
#include "halo2/native_tu15_pair_creation_queue_binding.hpp"
#include "halo2/native_tu15_activation_manager_factory_binding.hpp"
#include "halo2/native_tu15_pair_island_wake_binding.hpp"
#include "halo2/native_tu15_body_deactivation_frame_binding.hpp"
#include "halo2/native_tu15_discrete_pair_binding.hpp"
#include "halo2/native_tu15_pair_deletion_native_scope.hpp"
#include "halo2/native_tu15_reversed_list_agent_destructor_binding.hpp"
#include "halo2/native_tu15_contact_body_update_binding.hpp"
#include "halo2/native_tu15_main_heap_release_binding.hpp"
#include "halo2/native_tu15_broad_phase_edges_binding.hpp"
#include <map>
namespace halo2 {
enum class NativeTu15WorldCounter {Operations,AddedActions,ActiveIslands,PendingDeletes,PendingUpdates,RetiredBodies};
enum class NativeTu15WorldFlag {CheckDeactivation,RegroupIslands,Integrating};
enum class NativeTu15WorldScalar {SolverInterval,ScaledSeconds,ScaledInverse};
enum class NativeTu15WorldStepOutput {Seconds,Inverse,ScaledSeconds,ScaledInverse,JacobianSeconds,JacobianInverse,IterationSeconds,IterationInverse};
enum class NativeTu15WorldProfile {Begin,IslandListeners,IslandEnd,BeforeEndListeners,EndListeners,End};
class NativeTu15WorldIntegrationStorage {
public:
  virtual ~NativeTu15WorldIntegrationStorage()=default;
  virtual NativeTu15IslandIntegrationStorage&islands()=0;
  virtual std::uint32_t count(NativeTu15PhysicsWorld,NativeTu15WorldCounter)const=0;
  virtual bool flag(NativeTu15PhysicsWorld,NativeTu15WorldFlag)const=0;
  virtual void clear_added_actions(NativeTu15PhysicsWorld)=0;
  virtual void integrating(NativeTu15PhysicsWorld,bool)=0;
  virtual NativeTu15PhysicsIsland island(NativeTu15PhysicsWorld,std::uint32_t)const=0;
  virtual void clear_island_activation(NativeTu15PhysicsIsland)=0;
  virtual bool has_island_listeners(NativeTu15PhysicsWorld)const=0;
  virtual NativeTu15PhysicsBody retired_body(NativeTu15PhysicsWorld,std::uint32_t)const=0;
  virtual std::uint16_t references(NativeTu15PhysicsBody)const=0;
  virtual void references(NativeTu15PhysicsBody,std::uint16_t,bool protected_reset)=0;
  virtual bool protected_reference(NativeTu15PhysicsBody)const=0;
  virtual void clear_retired_bodies(NativeTu15PhysicsWorld)=0;
  virtual std::uint32_t seconds(NativeTu15PhysicsStepPacket)const=0;
  virtual std::uint32_t inverse_seconds(NativeTu15PhysicsStepPacket)const=0;
  virtual std::uint32_t scalar(NativeTu15PhysicsWorld,NativeTu15WorldScalar)const=0;
  virtual std::int32_t solver_iterations(NativeTu15PhysicsWorld)const=0;
  virtual void step_output(NativeTu15PhysicsWorld,NativeTu15WorldStepOutput,std::uint32_t)=0;
  virtual std::uint32_t profile_cursor()const=0;
  virtual std::uint32_t profile_limit()const=0;
  virtual void profile_label(std::uint32_t,NativeTu15WorldProfile)=0;
  virtual void profile_counter(std::uint32_t,std::uint32_t,NativeTu15WorldProfile)=0;
  virtual void profile_cursor(std::uint32_t,NativeTu15WorldProfile)=0;
};
struct NativeTu15WorldIntegrationServices {
  std::function<std::uint64_t()> timestamp_counter;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)> drain_operations;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)> add_actions;
  std::function<bool(NativeTu15PhysicsIsland,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&)> check_deactivation;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&)> deactivate_island;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&)> notify_island_listeners;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)> regroup_islands;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)> drain_deletes;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsFrameFp&)> drain_updates;
  std::function<void(NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&)> detach_body;
  std::function<void(NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&)> destroy_body;
  std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&)> notify_world_listeners;
  NativeTu15IslandIntegrationServices island;
};
struct NativeTu15WorldIntegrationStats {std::uint64_t world_integrations{},native_pose_calls{},native_deactivation_calls{};
  std::uint64_t native_kinematic_pose_calls{},native_fixed_pose_calls{},native_prescribed_force_calls{},native_body_policy_calls{},native_island_transition_calls{};
  std::uint64_t native_array_growth_calls{},native_array_pool_allocations{},native_array_pool_releases{};
  std::uint64_t native_array_reserve_calls{};
  std::uint64_t native_world_collision_calls{};
  std::uint64_t native_world_collision_listener_calls{},native_island_collision_listener_calls{};
  std::uint64_t native_world_step_listener_calls{},native_island_step_listener_calls{};
  std::uint64_t native_constraint_calls{},native_dynamic_accumulator_calls{},native_constraint_setup_calls{};
  std::uint64_t native_island_collision_calls{};
  std::uint64_t native_motion_commit_calls{};
  std::array<std::uint64_t,3> native_motion_commit_kinds{};
  std::uint64_t native_broad_phase_calls{};
  NativeTu15BroadPhaseEdgeBindingActivity broad_phase_edges;
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
  std::uint64_t native_wrapped_factory_attempts{},native_wrapped_factory_calls{},native_wrapped_factory_refusals{},native_wrapped_factory_wrapped_entries{},native_wrapped_factory_transformed_entries{},native_wrapped_factory_convex_entries{},native_wrapped_factory_class_queries{},native_wrapped_factory_unqualified_owners{};
  std::uint64_t native_contact_agent_cleanup_calls{},native_contact_agent_record_cleanups{},native_contact_agent_pointer_cleanups{},native_contact_reference_disposals{},native_contact_agent_child_residuals{},native_contact_agent_destructor_residuals{},native_contact_agent_cleanup_refusals{};
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
  std::uint64_t native_constraint_producer_axis_calls{},native_constraint_producer_scaled_calls{},
    native_constraint_producer_base_calls{},native_constraint_producer_base_rows{},
    native_constraint_producer_property_lookups{};
  std::uint64_t native_constraint_direct_base_calls{},native_constraint_direct_base_rows{};
  std::array<std::uint64_t,3> native_constraint_producer_emissions{},
    native_constraint_producer_completed_emissions{};
  // Diagnostic counts of actual named residual attempts. Native kernels do
  // not execute these identities; the explicit migration service owns them.
  std::map<std::uint32_t,std::uint64_t> residual_owner_calls,shape_bounds_residual_owners;
  std::uint64_t native_solver_custom_calls{};
  std::uint64_t native_solver_clock_calls{};
  std::array<std::uint64_t,3> native_solver_custom_kinds{};
  std::uint64_t native_solver_calls{},native_solver_initializations{},native_solver_iterations{},native_solver_primary_rows{},native_solver_secondary_rows{},native_solver_exported_rows{},native_solver_custom_refusals{};
  NativeTu15ContactProcessBindingStats contact_process;
  NativeTu15IslandAgentPhaseStats island_agents;
  NativeTu15IslandContactAdvanceStats contact_advance;
  NativeTu15CachedMotionStats cached_motion;
  NativeTu15ActivationManagerFactoryBindingActivity pair_manager_factory;
  NativeTu15PairCreationQueueBindingActivity pair_creation_queues;
  NativeTu15IslandRegroupBindingActivity island_regroup;
  NativeTu15IslandMergeStats island_merge;
  NativeTu15IslandContactMergeStats island_contact_merge;
  NativeTu15IslandLifetimeStats island_lifetime;
  NativeTu15ReversedListAgentDestructorActivity reversed_list_agent_destructor;
  NativeTu15PairDeletionBindingActivity pair_deletion;
  NativeTu15PairDeletionNativeScopeActivity pair_deletion_scope;
  NativeTu15PairIslandWakeActivity pair_island_wake;
  NativeTu15BodyDeactivationFrameBindingActivity body_deactivation;
  NativeTu15DiscretePairBindingActivity pair_discrete_query;
  NativeTu15ObjectActivationBindingActivity world_activation;
  std::uint64_t native_world_merge_refusals{},native_world_activation_calls{},native_world_activation_refusals{};
  NativeTu15WorldDeferredStats deferred;
  NativeTu15BodyReferenceStats body_references;
  NativeTu15ContactCompletionStats contact_completion;
  NativeTu15ContactEntityNotificationStats contact_entities;
  NativeTu15ContactEntityProcessorStats contact_processor;
  NativeTu15MainHeapReleaseBindingActivity main_heap_release;
  NativeTu15ContactBodyUpdateBindingActivity contact_body;
  std::uint64_t contact_body_wake_attempts{},contact_body_wake_calls{},contact_body_wake_refusals{},contact_body_wake_unqualified_owners{};
  std::map<std::uint32_t,std::uint64_t> contact_body_wake_refused_owners;
  NativeTu15GameplayDeactivationPolicyStats gameplay_deactivation;
  NativeTu15InactiveIslandCleanupStats inactive_island_cleanup;
  NativeTu15BodyPolicyStats inactive_island_policy;
  std::uint64_t native_inactive_island_cleanup_refusals{},inactive_island_policy_kind_residuals{};
  NativeTu15SphereBoundsStats sphere_bounds;
  std::uint64_t native_contact_fixed_motion_calls{},contact_motion_residual_calls{};
  NativeTu15ShapeBoundsStats shape_bounds;
  NativeTu15CenteredShapeBoundsStats centered_shape_bounds;
  NativeTu15CompoundBoundsStats compound_shape_bounds;
  std::uint64_t shape_bounds_residual_calls{};
  NativeTu15IslandIntegrationStats island;};
// Proper native actual319F90 integration, including original extended-precision
// step scaling, reverse active-island scheduling, native island/bodyforce path,
// retirement/reference ownership and listener timing. Named residual modules
// remain explicit; this does not certify a native-only solver/world closure.
void native_tu15_integrate_physics_world(NativeTu15WorldIntegrationStorage&,
  NativeTu15PhysicsWorld,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&,
  unsigned actual_x87_depth,const NativeTu15WorldIntegrationServices&,
  NativeTu15WorldIntegrationStats* = nullptr);
class NativeTu15WorldIntegrationMemory final:public NativeTu15WorldIntegrationStorage {
  SourcePhysicsContactMemory&memory_;SourcePhysicsContactRecordObserver observer_;
  NativeTu15IslandIntegrationMemory islands_;
  std::uint32_t read(std::uint32_t,unsigned=4)const;
  void write(std::uint32_t,std::uint32_t,std::uint32_t,unsigned=4);
public:
  NativeTu15WorldIntegrationMemory(SourcePhysicsContactMemory&,const SourcePhysicsContactRecordObserver& = {});
  NativeTu15IslandIntegrationStorage&islands()override;
  std::uint32_t count(NativeTu15PhysicsWorld,NativeTu15WorldCounter)const override;
  bool flag(NativeTu15PhysicsWorld,NativeTu15WorldFlag)const override;
  void clear_added_actions(NativeTu15PhysicsWorld)override;
  void integrating(NativeTu15PhysicsWorld,bool)override;
  NativeTu15PhysicsIsland island(NativeTu15PhysicsWorld,std::uint32_t)const override;
  void clear_island_activation(NativeTu15PhysicsIsland)override;
  bool has_island_listeners(NativeTu15PhysicsWorld)const override;
  NativeTu15PhysicsBody retired_body(NativeTu15PhysicsWorld,std::uint32_t)const override;
  std::uint16_t references(NativeTu15PhysicsBody)const override;
  void references(NativeTu15PhysicsBody,std::uint16_t,bool)override;
  bool protected_reference(NativeTu15PhysicsBody)const override;
  void clear_retired_bodies(NativeTu15PhysicsWorld)override;
  std::uint32_t seconds(NativeTu15PhysicsStepPacket)const override;
  std::uint32_t inverse_seconds(NativeTu15PhysicsStepPacket)const override;
  std::uint32_t scalar(NativeTu15PhysicsWorld,NativeTu15WorldScalar)const override;
  std::int32_t solver_iterations(NativeTu15PhysicsWorld)const override;
  void step_output(NativeTu15PhysicsWorld,NativeTu15WorldStepOutput,std::uint32_t)override;
  std::uint32_t profile_cursor()const override;
  std::uint32_t profile_limit()const override;
  void profile_label(std::uint32_t,NativeTu15WorldProfile)override;
  void profile_counter(std::uint32_t,std::uint32_t,NativeTu15WorldProfile)override;
  void profile_cursor(std::uint32_t,NativeTu15WorldProfile)override;
};
} // namespace halo2
