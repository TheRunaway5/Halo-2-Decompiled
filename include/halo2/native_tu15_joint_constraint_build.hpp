#pragma once
#include "halo2/native_tu15_joint_constraint_motor.hpp"
#include "halo2/native_tu15_joint_constraint_limits.hpp"
#include "halo2/native_tu15_joint_constraint_schema.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15JointConstraintBuildServices {
    // Both four-lane points are initialized source transformed arithmetic
    // DATA, including W. The owner executes in the already active native FP
    // environment; no original private frame or scalar padding is provided.
    std::function<void(const NativeTu15ConstraintVector4&,const NativeTu15ConstraintVector4&,
        NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,unsigned)> linear;
};
struct NativeTu15JointConstraintBuildStats {
    std::uint64_t calls{},completed{},headers{},angular_rows{},motors{},linear_requests{},
        limits{},positive_unwraps{},negative_unwraps{},refusals{},unbound_linear{};
};
// Actual31D930 semantic parent. Constraint/input/packet identities are genuine
// DATA formals. All intermediate frames/points/rows are initialized native
// values; no GP/SIMD/control-frame image is serialized or accepted.
void native_tu15_build_joint_constraint(SourcePhysicsContactMemory&,
    std::uint32_t constraint,NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15JointConstraintBuildServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15JointConstraintBuildStats* = nullptr);
} // namespace halo2
