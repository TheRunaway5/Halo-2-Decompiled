#pragma once
#include "halo2/native_tu15_joint_constraint_angular_row.hpp"
namespace halo2 {
struct NativeTu15JointConstraintMotor {
    NativeTu15ConstraintVector4 axis;
    std::array<std::uint32_t,3> parameters;
};
// Actual329100. The public28B input contains a16B axis and three float words.
// No input identity is retained in the generated Jacobian/schema DATA.
void native_tu15_emit_joint_constraint_motor(SourcePhysicsContactMemory&,
    std::uint32_t motor,NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
// Native parent composition with an already active hardware FP environment.
void native_tu15_emit_joint_constraint_motor_active(SourcePhysicsContactMemory&,
    const NativeTu15JointConstraintMotor&,NativeTu15ConstraintProducerInput,
    NativeTu15ConstraintSchemaPacket,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
