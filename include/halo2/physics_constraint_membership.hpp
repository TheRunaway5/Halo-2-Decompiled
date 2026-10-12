#pragma once
#include "halo2/physics_contact_batches.hpp"

namespace halo2 {
// Complete3136B0 deferred island-merge request. The source queues actual body
// identities; it does not perform a merge or claim a solved constraint.
void source_queue_physics_island_merge_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world,std::uint32_t first,std::uint32_t second,
    const SourcePhysicsContactRecordObserver & = {});
// Complete322320/31C520 original requirement producer and island accumulation.
// Actual source storage and constraint count are consumed; no solver size guess.
void source_physics_constraint_requirements_1_1(SourcePhysicsContactMemory &,
    std::uint32_t constraint,std::uint32_t output,
    const SourcePhysicsContactRecordObserver & = {});
//319AC0 authored ragdoll constraint schema/storage requirements.
void source_physics_ragdoll_constraint_requirements_1_1(
    SourcePhysicsContactMemory &,std::uint32_t output,
    const SourcePhysicsContactRecordObserver & = {});
//319090 limited-hinge requirements, including the authored motor branch.
void source_physics_hinge_constraint_requirements_1_1(
    SourcePhysicsContactMemory &,std::uint32_t constraint,std::uint32_t output,
    const SourcePhysicsContactRecordObserver & = {});
void source_accumulate_physics_constraint_requirements_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island,std::uint32_t constraint,std::uint32_t requirements,
    const SourcePhysicsContactRecordObserver & = {});
// Complete31D2B0 insertion and original refcount/island assignment. Growth and
// unknown constraint/island virtuals require their real owner, never success.
void source_insert_physics_constraint_into_island_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island,std::uint32_t constraint,std::uint32_t entry_esp,
    const SourcePhysicsContactBatchServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete31E7D0 reverse listener delivery and actual NULL compaction.
void source_notify_physics_constraint_added_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world,std::uint32_t constraint,std::uint32_t entry_esp,
    const SourcePhysicsContactBatchServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete313800 world/static/island ownership, with native3136B0 deferred
// merge requests. Actual activation3131C0 remains required when reached.
// Queueing a merge does not establish its execution or constraint solving.
std::uint32_t source_insert_physics_constraint_into_world_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world,std::uint32_t constraint,std::uint32_t entry_esp,
    const SourcePhysicsContactBatchServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
