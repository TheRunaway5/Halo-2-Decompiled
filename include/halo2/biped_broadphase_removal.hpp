#pragma once
#include "halo2/physics_broadphase.hpp"
namespace halo2 {
// Original2E6520 endpoint deletion, including overlapping forward MOVSD and
// the source count update between the two copy ranges.
void source_remove_physics_broadphase_axis_1_1(
    SourcePhysicsContactMemory &, std::uint32_t axis, std::uint32_t minimum,
    std::uint32_t maximum, const SourcePhysicsContactRecordObserver & = {});
// Original2E6340 signed packed-word repair after deletion. Both whole 64-bit
// stores retain the body's untouched handle word.
void source_repair_removed_physics_broadphase_indices_1_1(
    SourcePhysicsContactMemory &, std::uint32_t body_bank, std::uint32_t count,
    std::uint32_t removed, const SourcePhysicsContactRecordObserver & = {});
// Complete2E8F00 removal: current endpoint/bucket overlap search, real removed
// pairs, index repair, swapped handle/bucket repair and source scratch
// lifetime. entry_esp is caller-owned ABI scratch, never an object/contact
// identity.
void source_remove_physics_broadphase_body_1_1(
    SourcePhysicsContactMemory &, std::uint32_t backend, std::uint32_t handle,
    std::uint32_t removed_pairs, std::uint32_t entry_esp,
    const SourcePhysicsBroadphaseServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
