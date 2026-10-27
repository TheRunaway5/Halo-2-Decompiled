#pragma once
#include "halo2/native_tu15_motion_forces.hpp"
namespace halo2 {
class NativeTu15MotionPoseStorage {
public:
  virtual ~NativeTu15MotionPoseStorage()=default;
  virtual NativeTu15PhysicsVector angular_velocity()const=0;
  virtual NativeTu15PhysicsVector linear_velocity()const=0;
  virtual NativeTu15PhysicsVector position()const=0;
  virtual NativeTu15PhysicsVector quaternion()const=0;
  virtual std::uint32_t quaternion_component(unsigned)const=0;
  virtual NativeTu15PhysicsVector local_center()const=0;
  virtual NativeTu15PhysicsVector rotation(unsigned)const=0;
  virtual NativeTu15PhysicsVector xyz_mask()const=0;
  virtual std::uint32_t seconds()const=0;
  virtual std::uint32_t radius()const=0;
  virtual std::uint32_t one()const=0;
  virtual std::uint32_t normalization_three()const=0;
  virtual std::uint32_t normalization_half()const=0;
  virtual void previous_linear_velocity(NativeTu15PhysicsVector)=0;
  virtual void previous_angular_velocity(NativeTu15PhysicsVector)=0;
  virtual void previous_position(NativeTu15PhysicsVector)=0;
  virtual void position(NativeTu15PhysicsVector)=0;
  virtual void angular_travel(std::uint32_t)=0;
  virtual void quaternion(NativeTu15PhysicsVector,bool normalized)=0;
  virtual void rotation_component(unsigned,std::uint32_t)=0;
  virtual void world_origin(NativeTu15PhysicsVector)=0;
  virtual void position_w_one()=0;
};
// Read-only observation of the actual native RSQRTSS input/output. This is
// never an estimate provider, replay hook, or permission to substitute a
// precise reciprocal square root. PentiumIII/Xbox estimate-bit qualification
// remains separate from original instructions run on this physical host.
using NativeTu15PoseRsqrtObservation=std::function<void(std::uint32_t input,std::uint32_t native_estimate)>;
// Complete actual325170 plus its closed323430/1E1010/37C5D0 dependencies.
// No guest CPU register file, flags interpreter, emulated instructions, residual
// math callbacks, or older-image implementation is used by this algorithm.
void native_tu15_integrate_motion_pose(NativeTu15MotionPoseStorage&,
  NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
  const NativeTu15PoseRsqrtObservation& = {});
class NativeTu15MotionPoseMemory final:public NativeTu15MotionPoseStorage {
  SourcePhysicsContactMemory&memory_;std::uint32_t motion_,step_;
  SourcePhysicsContactRecordObserver observer_;
  std::uint32_t scalar(std::uint32_t)const;
  NativeTu15PhysicsVector vector(std::uint32_t)const;
  void scalar(std::uint32_t,std::uint32_t,std::uint32_t);
  void vector(std::uint32_t,std::uint32_t,const NativeTu15PhysicsVector&);
public:
  NativeTu15MotionPoseMemory(SourcePhysicsContactMemory&,std::uint32_t motion,std::uint32_t step,const SourcePhysicsContactRecordObserver& = {});
  NativeTu15PhysicsVector angular_velocity()const override;
  NativeTu15PhysicsVector linear_velocity()const override;
  NativeTu15PhysicsVector position()const override;
  NativeTu15PhysicsVector quaternion()const override;
  std::uint32_t quaternion_component(unsigned)const override;
  NativeTu15PhysicsVector local_center()const override;
  NativeTu15PhysicsVector rotation(unsigned)const override;
  NativeTu15PhysicsVector xyz_mask()const override;
  std::uint32_t seconds()const override;
  std::uint32_t radius()const override;
  std::uint32_t one()const override;
  std::uint32_t normalization_three()const override;
  std::uint32_t normalization_half()const override;
  void previous_linear_velocity(NativeTu15PhysicsVector)override;
  void previous_angular_velocity(NativeTu15PhysicsVector)override;
  void previous_position(NativeTu15PhysicsVector)override;
  void position(NativeTu15PhysicsVector)override;
  void angular_travel(std::uint32_t)override;
  void quaternion(NativeTu15PhysicsVector,bool)override;
  void rotation_component(unsigned,std::uint32_t)override;
  void world_origin(NativeTu15PhysicsVector)override;
  void position_w_one()override;
};
} // namespace halo2
