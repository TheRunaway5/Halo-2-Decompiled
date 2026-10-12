#pragma once
#include "halo2/unit_animation_selection_runtime.hpp"
#include "halo2/death_camera.hpp"

namespace halo2 {
// Complete16B240 and its original geometry dispatch over retained source banks.
// Segment arguments are addresses of the caller's actual three-float vectors.
// Result is the original0x5C record. World/object/pose banks remain memory-owned.
void source_run_segment_collision_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceSegmentCollisionContext {
  std::uint32_t entry_esp, fs_base;
};
struct SourceSegmentCollisionServices {
  SourceUnitAnimationRuntimeServices runtime;
  std::uint32_t fs_base;
};
bool source_segment_collision_1_1(
    SourcePhysicsContactMemory &, std::uint32_t flags, std::uint32_t start,
    std::uint32_t delta, std::uint32_t excluded_object,
    std::uint32_t excluded_parent, std::uint32_t result,
    SourceSegmentCollisionContext, SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceDeathCameraCollisionScratch {
  std::uint32_t origin, delta, result, entry_esp, fs_base;
};
// Binds1732B0's collision service directly to complete16B240. Caller-owned
// input/result/stack storage and all current world banks must remain alive.
// Both exclusion handles are original NONE; a miss comes only from16B240.
DeathCameraCollision source_death_camera_collision_1_1(
    SourcePhysicsContactMemory &, SourceDeathCameraCollisionScratch,
    SourcePhysicsFactoryFloatingPoint, SourceUnitAnimationRuntimeServices,
    SourcePhysicsContactRecordObserver = {});
} // namespace halo2
