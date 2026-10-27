#pragma once
#include "halo2/native_tu15_motion_pose.hpp"
namespace halo2 {
struct NativeTu15MotionVariantActivity {
  std::uint64_t kinematic_pose_calls{},prescribed_force_calls{},fixed_pose_calls{};
};
class NativeTu15KinematicPoseStorage {
public:
  virtual ~NativeTu15KinematicPoseStorage()=default;
  virtual NativeTu15PhysicsVector angular_velocity()const=0;
  virtual NativeTu15PhysicsVector linear_velocity()const=0;
  virtual NativeTu15PhysicsVector position()const=0;
  virtual NativeTu15PhysicsVector quaternion()const=0;
  virtual std::uint32_t quaternion_component(unsigned)const=0;
  virtual NativeTu15PhysicsVector local_center()const=0;
  virtual NativeTu15PhysicsVector xyz_mask()const=0;
  virtual std::uint32_t seconds()const=0;
  virtual std::uint32_t radius()const=0;
  virtual std::uint32_t one()const=0;
  virtual std::uint32_t normalization_three()const=0;
  virtual std::uint32_t normalization_half()const=0;
  virtual void previous_position(NativeTu15PhysicsVector)=0;
  virtual void position(NativeTu15PhysicsVector)=0;
  virtual void angular_travel(std::uint32_t)=0;
  virtual void quaternion(NativeTu15PhysicsVector,bool normalized)=0;
  virtual void rotation_component(unsigned,std::uint32_t)=0;
  virtual void world_origin(NativeTu15PhysicsVector)=0;
  virtual void position_w_one()=0;
};
// Actual323730 and its closed323430/1E1010/37C5D0/2E5810 chain. The source
// integrates prescribed velocities; it does not accumulate dynamic forces or
// write the dynamic motion's previous linear/angular velocity fields.
void native_tu15_integrate_kinematic_motion_pose(NativeTu15KinematicPoseStorage&,
  NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
  const NativeTu15PoseRsqrtObservation& = {},NativeTu15MotionVariantActivity* = nullptr);
// Authenticated08FDB0 RET8 and2E4D60 RET4 bodies perform no memory or floating
// point work. Their caller owns the real ABI protocol; these semantic methods
// preserve the supplied state and only record optional native participation.
void native_tu15_apply_prescribed_motion_forces(NativeTu15PhysicsFrameFp&,
  NativeTu15MotionVariantActivity* = nullptr);
void native_tu15_keep_fixed_motion_pose(NativeTu15PhysicsFrameFp&,
  NativeTu15MotionVariantActivity* = nullptr);
class NativeTu15KinematicPoseMemory final:public NativeTu15KinematicPoseStorage {
  SourcePhysicsContactMemory&memory_;std::uint32_t motion_,step_;
  SourcePhysicsContactRecordObserver observer_;
  std::uint32_t scalar(std::uint32_t)const;
  NativeTu15PhysicsVector vector(std::uint32_t)const;
  void scalar(std::uint32_t,std::uint32_t,std::uint32_t);
  void vector(std::uint32_t,std::uint32_t,const NativeTu15PhysicsVector&);
public:
  NativeTu15KinematicPoseMemory(SourcePhysicsContactMemory&,std::uint32_t motion,
    std::uint32_t step,const SourcePhysicsContactRecordObserver& = {});
  NativeTu15PhysicsVector angular_velocity()const override;
  NativeTu15PhysicsVector linear_velocity()const override;
  NativeTu15PhysicsVector position()const override;
  NativeTu15PhysicsVector quaternion()const override;
  std::uint32_t quaternion_component(unsigned)const override;
  NativeTu15PhysicsVector local_center()const override;
  NativeTu15PhysicsVector xyz_mask()const override;
  std::uint32_t seconds()const override;
  std::uint32_t radius()const override;
  std::uint32_t one()const override;
  std::uint32_t normalization_three()const override;
  std::uint32_t normalization_half()const override;
  void previous_position(NativeTu15PhysicsVector)override;
  void position(NativeTu15PhysicsVector)override;
  void angular_travel(std::uint32_t)override;
  void quaternion(NativeTu15PhysicsVector,bool normalized)override;
  void rotation_component(unsigned,std::uint32_t)override;
  void world_origin(NativeTu15PhysicsVector)override;
  void position_w_one()override;
};
} // namespace halo2
