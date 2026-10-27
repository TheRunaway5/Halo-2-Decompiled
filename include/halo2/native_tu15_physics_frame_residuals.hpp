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
  std::uint64_t frame_calls{},residual_calls{};
  std::uint64_t world_integrations{},island_integrations{},native_force_calls{},native_pose_calls{},native_deactivation_calls{},world_residual_calls{};
  std::uint64_t native_kinematic_pose_calls{},native_fixed_pose_calls{},native_prescribed_force_calls{},native_body_policy_calls{},native_island_transition_calls{};
  std::uint64_t native_array_growth_calls{},native_array_pool_allocations{},native_array_pool_releases{};
  std::uint64_t native_array_reserve_calls{};
  std::uint64_t native_world_collision_calls{};
  std::uint64_t native_world_collision_listener_calls{},native_island_collision_listener_calls{};
  std::uint64_t native_constraint_calls{},native_dynamic_accumulator_calls{},native_constraint_setup_calls{};
  std::uint64_t native_island_collision_calls{};
  std::uint64_t native_island_agent_flag_clear_calls{},native_island_agent_flag_set_calls{},
    native_island_agent_process_calls{},native_island_agent_finish_calls{},
    native_island_contact_minimum_calls{},native_island_contact_time_calls{},
    native_island_contact_buffers_constructed{},native_island_contact_buffers_destroyed{};
  std::uint64_t native_contact_advance_calls{},native_contact_advance_motion_calls{},
    native_contact_advance_process_calls{},native_contact_advance_single_iterations{},
    native_contact_advance_paired_iterations{},native_contact_advance_negative_single_restores{},
    native_contact_advance_negative_paired_restores{};
  std::uint64_t native_motion_commit_calls{};
  std::array<std::uint64_t,3> native_motion_commit_kinds{};
  std::uint64_t native_broad_phase_calls{};
  std::uint64_t native_shape_bounds_box_calls{},native_shape_bounds_convex_calls{},
    native_shape_bounds_transformed_calls{},native_shape_bounds_forward_calls{},native_shape_bounds_motion_calls{},
    native_shape_bounds_centered_box_calls{},native_shape_bounds_centered_radius_calls{},shape_bounds_residual_calls{};
  std::uint64_t native_shape_bounds_endpoint_calls{},native_shape_bounds_compound_calls{},
    native_shape_bounds_compound_child_calls{};
  std::uint64_t native_constraint_producer_axis_calls{},native_constraint_producer_scaled_calls{},
    native_constraint_producer_base_calls{},native_constraint_producer_base_rows{},
    native_constraint_producer_property_lookups{};
  std::array<std::uint64_t,3> native_constraint_producer_emissions{},
    native_constraint_producer_completed_emissions{};
  // Diagnostic counts of actual named residual attempts. Native kernels do
  // not execute these identities; the explicit migration service owns them.
  std::map<std::uint32_t,std::uint64_t> residual_owner_calls,shape_bounds_residual_owners;
  std::uint64_t native_solver_custom_calls{};
  std::uint64_t native_solver_clock_calls{};
  std::array<std::uint64_t,3> native_solver_custom_kinds{};
  std::uint64_t native_solver_calls{},native_solver_initializations{},native_solver_iterations{},native_solver_primary_rows{},native_solver_secondary_rows{},native_solver_exported_rows{},native_solver_custom_refusals{};
  std::array<std::uint64_t,4> native_lift_scalar_calls{};
  std::uint64_t native_lift_power_calls{},native_lift_errno_writes{},native_lift_observation_refusals{};
  std::uint64_t native_vehicle_rate_acceleration_calls{},native_vehicle_rate_approach_calls{},
    native_vehicle_rate_cursor_calls{},native_vehicle_rate_refusals{};
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
