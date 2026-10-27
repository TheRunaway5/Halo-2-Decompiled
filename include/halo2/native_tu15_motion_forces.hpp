#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
namespace halo2 {
using NativeTu15PhysicsVector=std::array<std::uint32_t,4>;
// Semantic motion view. Individual reads and persistent writes occur at their
// native algorithm phases; construction does not eagerly snapshot a body.
class NativeTu15MotionForceStorage {
public:
  virtual ~NativeTu15MotionForceStorage()=default;
  virtual std::uint32_t seconds()const=0;
  virtual std::uint32_t inverse_mass()const=0;
  virtual std::uint32_t linear_damping()const=0;
  virtual std::uint32_t angular_damping()const=0;
  virtual std::uint32_t one()const=0;
  virtual std::uint32_t zero()const=0;
  virtual std::uint32_t torque_present()const=0;
  virtual NativeTu15PhysicsVector gravity()const=0;
  virtual NativeTu15PhysicsVector linear_velocity()const=0;
  virtual NativeTu15PhysicsVector angular_velocity()const=0;
  virtual NativeTu15PhysicsVector accumulated_force()const=0;
  virtual NativeTu15PhysicsVector accumulated_torque()const=0;
  virtual NativeTu15PhysicsVector rotation(unsigned axis)const=0;
  virtual NativeTu15PhysicsVector inverse_inertia()const=0;
  virtual void linear_velocity(NativeTu15PhysicsVector,unsigned phase)=0;
  virtual void angular_velocity(NativeTu15PhysicsVector,bool impulse)=0;
  virtual void clear_force()=0;
  virtual void clear_torque()=0;
};
// Complete actual TU1.5 force3252D0 and angular impulse3250A0. There are no
// residual calls, instruction dispatchers, estimated reciprocal operations,
// virtual GPR files, or generic guest execution inside these native kernels.
// Live x87 values below the owner's three temporary slots are untouched. The
// caller must declare their actual count; overflow/fault environments diagnose
// unsupported platform ownership rather than manufacturing physics outputs.
void native_tu15_apply_motion_forces(NativeTu15MotionForceStorage&,
  NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth);
void native_tu15_apply_motion_angular_impulse(NativeTu15MotionForceStorage&,
  const NativeTu15PhysicsVector&,NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth);
class NativeTu15MotionForceMemory final:public NativeTu15MotionForceStorage {
  SourcePhysicsContactMemory&memory_;
  std::uint32_t motion_,step_,gravity_;
  SourcePhysicsContactRecordObserver observer_;
  std::uint32_t scalar(std::uint32_t)const;
  NativeTu15PhysicsVector vector(std::uint32_t)const;
  void vector(std::uint32_t,std::uint32_t,const NativeTu15PhysicsVector&);
public:
  NativeTu15MotionForceMemory(SourcePhysicsContactMemory&,std::uint32_t motion,
    std::uint32_t step,std::uint32_t gravity,const SourcePhysicsContactRecordObserver& = {});
  std::uint32_t seconds()const override;
  std::uint32_t inverse_mass()const override;
  std::uint32_t linear_damping()const override;
  std::uint32_t angular_damping()const override;
  std::uint32_t one()const override;
  std::uint32_t zero()const override;
  std::uint32_t torque_present()const override;
  NativeTu15PhysicsVector gravity()const override;
  NativeTu15PhysicsVector linear_velocity()const override;
  NativeTu15PhysicsVector angular_velocity()const override;
  NativeTu15PhysicsVector accumulated_force()const override;
  NativeTu15PhysicsVector accumulated_torque()const override;
  NativeTu15PhysicsVector rotation(unsigned)const override;
  NativeTu15PhysicsVector inverse_inertia()const override;
  void linear_velocity(NativeTu15PhysicsVector,unsigned)override;
  void angular_velocity(NativeTu15PhysicsVector,bool)override;
  void clear_force()override;
  void clear_torque()override;
};
} // namespace halo2
