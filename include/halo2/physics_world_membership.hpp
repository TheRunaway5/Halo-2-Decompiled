#pragma once
#include "halo2/physics_body_state.hpp"

namespace halo2 {
// Actual world/island/body records in the caller's allocator-owned storage.
// These owners establish membership and indices; they do not make a contact.
std::uint32_t source_construct_physics_world_island_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island,std::uint32_t world,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_append_physics_world_island_body_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island,std::uint32_t body,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_commit_physics_world_membership_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world,std::uint32_t body,
    const SourcePhysicsContactRecordObserver & = {});

struct SourcePhysicsWorldInsertionServices {
    // Required complete3144D0 owner, including real current-shape bounds and
    // backend broadphase insertion/pairs. No successful/default output.
    std::function<void(SourcePhysicsContactMemory &,std::uint32_t world,
        std::uint32_t body,const SourcePhysicsContactRecordObserver &)> broadphase;
    // Actual31E610 virtual +0C callback with the retained body argument.
    std::function<void(SourcePhysicsContactMemory &,std::uint32_t target,
        std::uint32_t listener,std::uint32_t body,
        const SourcePhysicsContactRecordObserver &)> world_listener;
};
void source_notify_physics_world_insertion_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world,std::uint32_t body,
    const SourcePhysicsWorldInsertionServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete315820 store/call order, with the real membership owner above.
// Missing broadphase stops at3144D0 after original preceding stores. World
// notification happens only after that required owner genuinely completes.
std::uint32_t source_insert_physics_world_body_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world,std::uint32_t body,
    const SourcePhysicsWorldInsertionServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
