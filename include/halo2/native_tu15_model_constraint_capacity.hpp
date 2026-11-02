#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15ModelConstraintCapacityServices {
    // Actual22D0A0 retains lifetime ownership until its native closure is
    // qualified. The native parent never retries its own original owner.
    std::function<void(std::uint32_t constraint_datum,NativeTu15PhysicsFrameFp&)>remove_constraint;
};
struct NativeTu15ModelConstraintCapacityStats {
    std::uint64_t calls{},slot_refusals{},local_full{},local_candidates{},
        local_removals{},global_removals{},stack_queries{},refusals{},unbound_removals{};
};
// Actual1DD7B0 and22DD90. Extent is the genuine incoming scalar value, passed
// as ordinary bits; no volatile GP/SIMD state belongs to this interface.
bool native_tu15_admit_model_constraint(SourcePhysicsContactMemory&,
    NativeTu15PhysicsComponent,std::uint32_t body_slot,std::uint32_t extent_bits,
    NativeTu15PhysicsFrameFp&,const NativeTu15ModelConstraintCapacityServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ModelConstraintCapacityStats* = nullptr);
} // namespace halo2
