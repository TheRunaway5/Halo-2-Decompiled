#pragma once
#include "halo2/physics_body_state.hpp"

namespace halo2 {
struct SourcePhysicsBodyVirtualRelease {
    std::uint32_t instruction, target, object;
    std::vector<std::uint32_t> arguments;
};
// Providers must execute the actual required owner on retained storage. They
// cannot return a captured address or a success flag. Empty providers stop at
// the original call after all preceding source stores have been delivered.
struct SourcePhysicsBodyDisposalServices {
    std::function<void(SourcePhysicsContactMemory &,
        const SourcePhysicsBodyVirtualRelease &,
        const SourcePhysicsContactRecordObserver &)> virtual_owner;
    std::function<void(SourcePhysicsContactMemory &,std::uint32_t,
        const SourcePhysicsContactRecordObserver &)> clear_reference_flag;
    // Read-only complete original allocator call: source instruction, owning
    // adapter, retained block, byte count and allocation class. No result.
    std::function<void(const std::array<std::uint32_t,5> &)> observe_free;
};
struct SourcePhysicsBodyDisposalScratch {
    std::uint32_t removed_property_2001, removed_property_2002;
};
void source_remove_physics_entity_property_1_1(SourcePhysicsContactMemory &,
    std::uint32_t entity,std::uint32_t output,std::uint32_t key,
    const SourcePhysicsContactRecordObserver & = {});
bool source_physics_body_reference_is_resident_1_1(
    const SourcePhysicsContactMemory &,std::uint32_t reference);
// Original317AD0/317970 and actual mode4/5/7 motion/auxiliary/base destructors,
// including the controller's retained wrapper and child shape retirement;
// real allocator virtual identity and backing are required at every free.
std::uint32_t source_destroy_physics_rigid_body_1_1(SourcePhysicsContactMemory &,
    std::uint32_t body,std::uint32_t flags,
    const SourcePhysicsBodyDisposalServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete1D3BB0 order, including real reference/property/bank retirement.
// Bank pointers/capacities survive; slot identities/counts do not. This does
// not remove/insert a body in a world and does not manufacture a contact.
std::uint32_t source_dispose_physics_component_bodies_1_1(SourcePhysicsContactMemory &,
    std::uint32_t component,const SourcePhysicsBodyDisposalScratch &,
    const SourcePhysicsBodyDisposalServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
