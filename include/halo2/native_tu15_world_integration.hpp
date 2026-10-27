#pragma once
#include "halo2/native_tu15_island_integration.hpp"
#include "halo2/native_tu15_island_agent_phase.hpp"
#include "halo2/native_tu15_island_contact_advance.hpp"
#include "halo2/native_tu15_compound_shape_bounds.hpp"
#include "halo2/native_tu15_centered_shape_bounds.hpp"
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
  std::uint64_t native_constraint_calls{},native_dynamic_accumulator_calls{},native_constraint_setup_calls{};
  std::uint64_t native_island_collision_calls{};
  std::uint64_t native_motion_commit_calls{};
  std::array<std::uint64_t,3> native_motion_commit_kinds{};
  std::uint64_t native_broad_phase_calls{};
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
  NativeTu15IslandAgentPhaseStats island_agents;
  NativeTu15IslandContactAdvanceStats contact_advance;
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
