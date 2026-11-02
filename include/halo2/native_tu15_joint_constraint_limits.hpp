#pragma once
#include "halo2/native_tu15_joint_constraint_angular_row.hpp"
namespace halo2 {
struct NativeTu15JointConstraintLimit {
    NativeTu15ConstraintVector4 axis;
    std::uint32_t values{},coefficient{};
};
// Actual328A00. The public16B block holds axis/limit identities, coefficient
// and signed source count; the generated rows retain no descriptor pointer.
void native_tu15_emit_joint_constraint_limits(SourcePhysicsContactMemory&,
    std::uint32_t descriptor,NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
// The sole31D930 native caller supplies one initialized axis/limit descriptor.
// Its values identity is genuine persistent limit DATA; no GP frame is built.
void native_tu15_emit_joint_constraint_limit_active(SourcePhysicsContactMemory&,
    const NativeTu15JointConstraintLimit&,NativeTu15ConstraintProducerInput,
    NativeTu15ConstraintSchemaPacket,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
