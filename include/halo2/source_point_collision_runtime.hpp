#pragma once
#include "halo2/source_segment_collision.hpp"

namespace halo2 {
// Complete16A9C0,16A8A0 hierarchy/exclusions and1F3780 posed containment.
// Actual current transforms and geometry are consumed from the retained bank.
void source_run_point_collision_runtime_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
bool source_world_point_collision_1_1(
    SourcePhysicsContactMemory &, std::uint32_t flags, std::uint32_t point,
    std::uint32_t excluded_a, std::uint32_t excluded_b,
    SourceSegmentCollisionContext, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
