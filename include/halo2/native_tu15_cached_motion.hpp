#pragma once
#include "halo2/native_tu15_constraint_step.hpp"
#include "halo2/native_tu15_motion_pose.hpp"
namespace halo2 {
struct NativeTu15CachedMotionStats {
    std::uint64_t calls{},rewinds{},advances{};
};
// Actual325810 and its closed323430/1E1010/37C5D0/2E5810 helpers.
// Rewind the current cached fraction, then advance to the actual new fraction.
// Both arguments are immutable source binary32 value bits. The source keeps
// the extended product for angular travel while separately spilling its
// linear/quaternion scale. No supplied pose or RSQRT estimate is an input.
// The closed call family reaches private x87 depth3; preserve at most5 values.
void native_tu15_advance_cached_motion(SourcePhysicsContactMemory&,
    NativeTu15PhysicsMotion,std::uint32_t fraction_bits,std::uint32_t seconds_bits,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},
    const NativeTu15PoseRsqrtObservation& = {},NativeTu15CachedMotionStats* = nullptr);
} // namespace halo2
