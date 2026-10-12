#pragma once
#include "halo2/biped_physics_activation.hpp"
#include "halo2/physics_body_disposal.hpp"
#include "halo2/physics_contact_batches.hpp"
#include "halo2/physics_pair_manager.hpp"

namespace halo2 {
struct SourceBipedWorldRemovalServices {
  SourcePhysicsContactBatchServices lower;
  SourceBipedPhysicsActivationServices activation;
  SourcePhysicsBodyDisposalServices disposal;
  SourcePhysicsBroadphaseServices scratch;
  SourcePhysicsPairManagerServices pairs;
};
// Original314690 composes current broadphase removal and original31D980 /
// 378EB0 pair delivery. Actual pair-agent retirement remains a required owner.
void source_remove_biped_world_broadphase_1_1(
    SourcePhysicsContactMemory &, std::uint32_t world, std::uint32_t body,
    std::uint32_t entry_esp, const SourceBipedWorldRemovalServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete313950/313870/3138E0 stable null compaction. Current source bank
// capacities and allocator own growth; no listener identity is supplied.
void source_compact_physics_body_listeners_1_1(
    SourcePhysicsContactMemory &, std::uint32_t body,
    const SourcePhysicsContactRecordObserver & = {});
// Complete31E570→31D810 deleting island owner, including every original bank
// retirement and real allocator call. This does not choose an island to remove.
void source_destroy_physics_world_island_1_1(
    SourcePhysicsContactMemory &, std::uint32_t island, std::uint32_t flags,
    const SourcePhysicsBodyDisposalServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete314810/31C5C0 membership retirement, swap indices, empty island
// deletion and deferred listener compaction. Unknown virtuals remain required.
void source_detach_physics_world_body_1_1(
    SourcePhysicsContactMemory &, std::uint32_t world, std::uint32_t body,
    std::uint32_t entry_esp, const SourceBipedWorldRemovalServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete315D50 deferred/direct routing and body reference lifetime, composed
// with native314690 broadphase removal. Actual pair retirement owners remain
// required when reached; an unavailable retirement is never a successful miss.
void source_remove_physics_world_body_1_1(
    SourcePhysicsContactMemory &, std::uint32_t world, std::uint32_t output,
    std::uint32_t body, std::uint32_t entry_esp,
    const SourceBipedWorldRemovalServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1D4C50 component removal orchestration, including constraints,
// listeners, attached-object wakeup, contact retirement and retained flags.
void source_remove_biped_component_world_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t entry_esp, const SourceBipedWorldRemovalServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
