#pragma once
#include "halo2/native_tu15_linear_jacobian_emitter.hpp"
#include "halo2/native_tu15_joint_constraint_angular_row.hpp"
namespace halo2 {
// Both source-transformed points have four initialized lanes, including W.
// The caller already owns the active native hardware FP environment. This
// shares the numeric public3294E0 algorithm, without guest-frame point DATA.
void native_tu15_emit_world_linear_jacobians_active(SourcePhysicsContactMemory&,
    const NativeTu15ConstraintVector4&first_point,const NativeTu15ConstraintVector4&second_point,
    std::uint32_t input,std::uint32_t packet,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15LinearJacobianEmitterStats* = nullptr);
} // namespace halo2
