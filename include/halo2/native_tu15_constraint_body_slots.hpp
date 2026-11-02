#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintBodySlots {std::uint32_t first{},second{};};
struct NativeTu15ConstraintBodySlotStats {
    std::uint64_t calls{},model_queries{},primary_queries{},child_rows{},matching_children{};
};
// Actual22E390/DE8C0. Resolve genuine existing constraint data to body slots;
// results are ordinary fields, never retained guest output registers.
NativeTu15ConstraintBodySlots native_tu15_constraint_body_slots(
    const SourcePhysicsContactMemory&,std::uint32_t instance,std::uint32_t constraint_datum,
    NativeTu15ConstraintBodySlotStats* = nullptr);
} // namespace halo2
