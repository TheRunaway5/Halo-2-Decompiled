#pragma once
#include "halo2/native_tu15_constraint_local_points.hpp"
#include "halo2/native_tu15_constraint_motion_compare.hpp"
#include "halo2/native_tu15_constraint_reference_release.hpp"
#include "halo2/native_tu15_vector_normalize.hpp"
namespace halo2 {
struct NativeTu15ModelConstraintCreationServices {
    NativeTu15ConstraintReferenceReleaseServices reference;
};
struct NativeTu15ModelConstraintCreationStats {
    std::uint64_t calls{},unsupported_numerical_profiles{},allocations{},second_definitions{},
        local_point_updates{},reference_releases{},created{};
    NativeTu15ModelConstraintUpdateStats distance;
    NativeTu15VectorNormalizeStats normalize;
    NativeTu15ConstraintMotionCompareStats motion;
    NativeTu15ConstraintLocalPointStats points;
    NativeTu15ConstraintReferenceReleaseStats reference;
};
// Actual22CEA0. Its sparse64-byte request is caller-owned ordinary DATA.
// Native122B60's five temporary x87 operands limit this composition to actual
// preserved depths0..3 and masked numerical faults. No original parent replay.
std::uint32_t native_tu15_create_model_constraint(SourcePhysicsContactMemory&,
    NativeTu15ModelConstraintRequest,std::uint32_t flags,
    NativeTu15ConstraintLocalPointScratch,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const NativeTu15ModelConstraintCreationServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ModelConstraintCreationStats* = nullptr);
} // namespace halo2
