#pragma once
#include "halo2/native_tu15_constraint_step.hpp"
#include "halo2/native_tu15_motion_pose.hpp"
namespace halo2 {
// Selected only after authenticating the actual motion vtable's +10 owner.
enum class NativeTu15MotionCommitKind {kinematic,dynamic,restore_saved_velocity};
class NativeTu15MotionCommitStorage:public NativeTu15MotionPoseStorage {
public:
    // This identity is the immutable declared accumulator argument, not a
    // memory read or a fabricated CPU return value.
    virtual NativeTu15ConstraintAccumulator accumulator()const=0;
    virtual NativeTu15PhysicsVector accumulator_working_linear()const=0;
    virtual NativeTu15PhysicsVector accumulator_working_angular()const=0;
    virtual NativeTu15PhysicsVector accumulator_saved_linear()const=0;
    virtual NativeTu15PhysicsVector accumulator_saved_angular()const=0;
    virtual void working_linear_velocity(NativeTu15PhysicsVector)=0;
    virtual void working_angular_velocity(NativeTu15PhysicsVector)=0;
    virtual void masked_linear_velocity(NativeTu15PhysicsVector)=0;
    virtual void masked_angular_velocity(NativeTu15PhysicsVector)=0;
    virtual void restored_linear_velocity(NativeTu15PhysicsVector)=0;
    virtual void restored_angular_velocity(NativeTu15PhysicsVector)=0;
};
// Complete actual3235F0/3249E0/3255D0 plus closed323430/1E1010/37C5D0
// and (for kinematic/dynamic commits)2E5810. The parameter is its actual
// binary32 argument bits; the kinematic owner never consumes it. Working
// accumulator lanes are genuine solver outputs, not supplied solver success.
// Actual native x87/SSE math is used, without a CPU register/flags machine.
// PentiumIII/Xbox RSQRT estimate bits and unmasked platform traps remain
// separately qualified, as for the existing native pose kernels.
NativeTu15ConstraintAccumulator native_tu15_commit_motion(
    NativeTu15MotionCommitKind,NativeTu15MotionCommitStorage&,
    std::uint32_t parameter_bits,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,
    const NativeTu15PoseRsqrtObservation& = {});
class NativeTu15MotionCommitMemory final:public NativeTu15MotionCommitStorage {
    SourcePhysicsContactMemory&memory_;NativeTu15MotionCommitKind kind_;
    std::uint32_t motion_,step_,accumulator_;
    SourcePhysicsContactRecordObserver observer_;
    std::uint32_t scalar(std::uint32_t)const;
    NativeTu15PhysicsVector vector(std::uint32_t)const;
    void scalar(std::uint32_t,std::uint32_t,std::uint32_t);
    void vector(std::uint32_t,std::uint32_t,const NativeTu15PhysicsVector&);
    std::uint32_t site(std::uint32_t,std::uint32_t,std::uint32_t)const;
public:
    NativeTu15MotionCommitMemory(SourcePhysicsContactMemory&,
        NativeTu15MotionCommitKind,NativeTu15PhysicsMotion,
        NativeTu15PhysicsStepPacket,NativeTu15ConstraintAccumulator,
        const SourcePhysicsContactRecordObserver& = {});
    NativeTu15ConstraintAccumulator accumulator()const override;
    NativeTu15PhysicsVector accumulator_working_linear()const override;
    NativeTu15PhysicsVector accumulator_working_angular()const override;
    NativeTu15PhysicsVector accumulator_saved_linear()const override;
    NativeTu15PhysicsVector accumulator_saved_angular()const override;
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
    void working_linear_velocity(NativeTu15PhysicsVector)override;
    void working_angular_velocity(NativeTu15PhysicsVector)override;
    void masked_linear_velocity(NativeTu15PhysicsVector)override;
    void masked_angular_velocity(NativeTu15PhysicsVector)override;
    void restored_linear_velocity(NativeTu15PhysicsVector)override;
    void restored_angular_velocity(NativeTu15PhysicsVector)override;
    void previous_linear_velocity(NativeTu15PhysicsVector)override;
    void previous_angular_velocity(NativeTu15PhysicsVector)override;
    void previous_position(NativeTu15PhysicsVector)override;
    void position(NativeTu15PhysicsVector)override;
    void angular_travel(std::uint32_t)override;
    void quaternion(NativeTu15PhysicsVector,bool normalized)override;
    void rotation_component(unsigned,std::uint32_t)override;
    void world_origin(NativeTu15PhysicsVector)override;
    void position_w_one()override;
};
} // namespace halo2
