#pragma once
#include "halo2/native_tu15_motion_forces.hpp"
#include <optional>
namespace halo2 {
struct NativeTu15PhysicsIsland {std::uint32_t identity{};};
struct NativeTu15PhysicsMotion {std::uint32_t identity{};};
struct NativeTu15PhysicsArray {std::uint32_t identity{};};
struct NativeTu15PhysicsStepConfiguration {std::uint32_t identity{};};
struct NativeTu15PhysicsStepPacket {std::uint32_t identity{};};
struct NativeTu15PhysicsGravity {std::uint32_t identity{};};
struct NativeTu15PhysicsAction {std::uint32_t identity{};};
struct NativeTu15PhysicsVirtualOwner {std::uint32_t identity{};};
struct NativeTu15PhysicsConstraintInput {
  NativeTu15PhysicsStepPacket step;
  std::uint32_t solver_configuration{},jacobian_context{};
  NativeTu15PhysicsGravity gravity;
  NativeTu15PhysicsIsland island;
  NativeTu15PhysicsArray bodies;
  std::uint32_t body_count{},constraints{},constraint_count{};
};
class NativeTu15IslandIntegrationStorage {
public:
  virtual ~NativeTu15IslandIntegrationStorage()=default;
  virtual NativeTu15PhysicsArray actions(NativeTu15PhysicsIsland)const=0;
  virtual std::uint32_t action_count(NativeTu15PhysicsIsland)const=0;
  virtual NativeTu15PhysicsAction action(std::uint32_t cursor)const=0;
  virtual NativeTu15PhysicsVirtualOwner action_owner(NativeTu15PhysicsAction)const=0;
  virtual std::uint32_t constraint_count(NativeTu15PhysicsIsland)const=0;
  virtual std::uint32_t constraints(NativeTu15PhysicsIsland)const=0;
  virtual std::uint32_t body_count(NativeTu15PhysicsIsland)const=0;
  virtual NativeTu15PhysicsArray bodies(NativeTu15PhysicsIsland)const=0;
  virtual NativeTu15PhysicsBody body(NativeTu15PhysicsArray,std::uint32_t)const=0;
  virtual NativeTu15PhysicsWorld world(NativeTu15PhysicsIsland)const=0;
  virtual NativeTu15PhysicsMotion motion(NativeTu15PhysicsBody)const=0;
  virtual NativeTu15PhysicsVirtualOwner force_owner(NativeTu15PhysicsMotion)const=0;
  virtual NativeTu15PhysicsVirtualOwner pose_owner(NativeTu15PhysicsMotion)const=0;
  virtual NativeTu15MotionForceStorage& motion_forces(NativeTu15PhysicsMotion,NativeTu15PhysicsStepPacket,NativeTu15PhysicsGravity)=0;
  virtual std::uint32_t profile_cursor()const=0;
  virtual std::uint32_t profile_limit()const=0;
  virtual void profile_label(std::uint32_t cursor,bool beginning)=0;
  virtual void profile_counter(std::uint32_t cursor,std::uint32_t ticks,bool beginning)=0;
  virtual void profile_cursor(std::uint32_t value,bool beginning)=0;
};
struct NativeTu15IslandIntegrationServices {
  std::function<std::uint64_t()> timestamp_counter;
  std::function<void(NativeTu15PhysicsAction,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&)> update_action;
  std::function<void(NativeTu15PhysicsMotion,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsStepPacket,NativeTu15PhysicsGravity,NativeTu15PhysicsFrameFp&)> apply_other_motion_forces;
  std::function<void(NativeTu15PhysicsMotion,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsStepPacket,NativeTu15PhysicsFrameFp&)> integrate_pose;
  std::function<void(const NativeTu15PhysicsConstraintInput&,NativeTu15PhysicsFrameFp&)> solve_constraints;
};
struct NativeTu15IslandIntegrationStats {std::uint64_t islands{},native_force_calls{},other_force_calls{},pose_calls{},constraint_calls{},action_calls{};};
// Complete actual TU1.5 island320B70: fixed action span, reverse body traversal,
// direct closed native force3252D0 for the authenticated dynamic-motion owner,
// and explicit named residual pose/action/constraint modules. Mixed execution
// remains unqualified as a full native-only physics closure.
void native_tu15_integrate_physics_island(NativeTu15IslandIntegrationStorage&,
  NativeTu15PhysicsIsland,NativeTu15PhysicsStepConfiguration,NativeTu15PhysicsFrameFp&,
  unsigned actual_x87_depth,const NativeTu15IslandIntegrationServices&,
  NativeTu15IslandIntegrationStats* = nullptr);
class NativeTu15IslandIntegrationMemory final:public NativeTu15IslandIntegrationStorage {
  SourcePhysicsContactMemory&memory_;
  SourcePhysicsContactRecordObserver observer_;
  std::optional<NativeTu15MotionForceMemory> force_;
  std::uint32_t read(std::uint32_t)const;
  void write(std::uint32_t,std::uint32_t,std::uint32_t);
public:
  NativeTu15IslandIntegrationMemory(SourcePhysicsContactMemory&,const SourcePhysicsContactRecordObserver& = {});
  NativeTu15PhysicsArray actions(NativeTu15PhysicsIsland)const override;
  std::uint32_t action_count(NativeTu15PhysicsIsland)const override;
  NativeTu15PhysicsAction action(std::uint32_t)const override;
  NativeTu15PhysicsVirtualOwner action_owner(NativeTu15PhysicsAction)const override;
  std::uint32_t constraint_count(NativeTu15PhysicsIsland)const override;
  std::uint32_t constraints(NativeTu15PhysicsIsland)const override;
  std::uint32_t body_count(NativeTu15PhysicsIsland)const override;
  NativeTu15PhysicsArray bodies(NativeTu15PhysicsIsland)const override;
  NativeTu15PhysicsBody body(NativeTu15PhysicsArray,std::uint32_t)const override;
  NativeTu15PhysicsWorld world(NativeTu15PhysicsIsland)const override;
  NativeTu15PhysicsMotion motion(NativeTu15PhysicsBody)const override;
  NativeTu15PhysicsVirtualOwner force_owner(NativeTu15PhysicsMotion)const override;
  NativeTu15PhysicsVirtualOwner pose_owner(NativeTu15PhysicsMotion)const override;
  NativeTu15MotionForceStorage& motion_forces(NativeTu15PhysicsMotion,NativeTu15PhysicsStepPacket,NativeTu15PhysicsGravity)override;
  std::uint32_t profile_cursor()const override;
  std::uint32_t profile_limit()const override;
  void profile_label(std::uint32_t,bool)override;
  void profile_counter(std::uint32_t,std::uint32_t,bool)override;
  void profile_cursor(std::uint32_t,bool)override;
};
} // namespace halo2
