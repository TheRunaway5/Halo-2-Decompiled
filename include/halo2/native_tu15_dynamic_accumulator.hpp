#pragma once
#include "halo2/native_tu15_constraint_step.hpp"
namespace halo2 {
class NativeTu15DynamicAccumulatorStorage:public NativeTu15MotionForceStorage {
public:
    virtual std::uint32_t rotation_component(unsigned)const=0;
    virtual void accumulator_rotation_component(unsigned,std::uint32_t,std::uint32_t site)=0;
    virtual NativeTu15PhysicsVector accumulator_rotation(unsigned)const=0;
    virtual NativeTu15PhysicsVector position()const=0;
    virtual void accumulator_type()=0;
    virtual void accumulator_inverse_inertia(NativeTu15PhysicsVector)=0;
    virtual void accumulator_linear_velocity(NativeTu15PhysicsVector)=0;
    virtual void accumulator_position(NativeTu15PhysicsVector)=0;
    virtual void accumulator_angular_velocity(NativeTu15PhysicsVector)=0;
    virtual void accumulator_dynamic_flag()=0;
    virtual NativeTu15ConstraintAccumulator output()const=0;
};
// Complete actual325410 plus ordered transpose2E4820, using the actual closed
// native3250A0 angular impulse only where the source calls it. No free-step
// force3252D0, guessed solver outcome, CPU file or instruction lift is used.
NativeTu15ConstraintAccumulator native_tu15_setup_dynamic_motion_accumulator(
    NativeTu15DynamicAccumulatorStorage&,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth);
class NativeTu15DynamicAccumulatorMemory final:public NativeTu15DynamicAccumulatorStorage {
    SourcePhysicsContactMemory&memory_;std::uint32_t motion_,step_,gravity_,output_;
    SourcePhysicsContactRecordObserver observer_;
    std::uint32_t scalar(std::uint32_t)const;
    NativeTu15PhysicsVector vector(std::uint32_t)const;
    void scalar(std::uint32_t,std::uint32_t,std::uint32_t,unsigned bytes=4);
    void vector(std::uint32_t,std::uint32_t,const NativeTu15PhysicsVector&);
public:
    NativeTu15DynamicAccumulatorMemory(SourcePhysicsContactMemory&,NativeTu15PhysicsMotion,
        NativeTu15PhysicsStepPacket,NativeTu15PhysicsGravity,NativeTu15ConstraintAccumulator,
        const SourcePhysicsContactRecordObserver& = {});
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
    std::uint32_t rotation_component(unsigned)const override;
    void accumulator_rotation_component(unsigned,std::uint32_t,std::uint32_t)override;
    NativeTu15PhysicsVector accumulator_rotation(unsigned)const override;
    NativeTu15PhysicsVector position()const override;
    void accumulator_type()override;
    void accumulator_inverse_inertia(NativeTu15PhysicsVector)override;
    void accumulator_linear_velocity(NativeTu15PhysicsVector)override;
    void accumulator_position(NativeTu15PhysicsVector)override;
    void accumulator_angular_velocity(NativeTu15PhysicsVector)override;
    void accumulator_dynamic_flag()override;
    NativeTu15ConstraintAccumulator output()const override;
};
} // namespace halo2
