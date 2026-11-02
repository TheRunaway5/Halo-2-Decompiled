#pragma once
#include "halo2/native_tu15_body_constraint_update.hpp"
#include "halo2/native_tu15_constraint_body_slots.hpp"
namespace halo2 {
struct NativeTu15ConstraintLocalPointScratch {
    // Two distinct52-byte ordinary DATA buffers owned by the caller. They
    // must not alias the instance or any retained model/motion input.
    std::uint32_t frame{},inverse{};
};
struct NativeTu15ConstraintLocalPointStats {
    std::uint64_t calls{},retained_node_positions{},first_points{},second_points{},inverse_frames{};
    NativeTu15ModelConstraintUpdateStats frames;
};
// Actual22EBA0, including native1D8B90/144090 and source scalar association.
// Body slots and scratch refer to real caller-owned DATA, never CPU frames.
void native_tu15_update_constraint_local_points(SourcePhysicsContactMemory&,
    std::uint32_t instance,NativeTu15ConstraintBodySlots,
    NativeTu15ConstraintLocalPointScratch,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintLocalPointStats* = nullptr);
} // namespace halo2
