#pragma once
#include "halo2/native_tu15_constraint_body_slots.hpp"
#include "halo2/native_tu15_constraint_scalar_cache.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15ConstraintReferenceReleaseServices {
    // Actual22FEA0 consumes its real instance through EBX and datum/slots
    // through stack arguments. These are semantic inputs, never GP results.
    std::function<void(std::uint32_t instance,std::uint32_t datum,
        NativeTu15ConstraintBodySlots,NativeTu15PhysicsFrameFp&)>remove_runtime_constraint;
};
struct NativeTu15ConstraintReferenceReleaseStats {
    std::uint64_t calls{},runtime_removals{},unbound_runtime_removals{},decrements{};
    NativeTu15ConstraintBodySlotStats slots;
    NativeTu15ConstraintScalarCacheStats cache;
};
// Actual22E1B0: finish eligible runtime ownership before the actual16-bit
// reference decrement. Missing lifetime ownership never replays this parent.
void native_tu15_release_constraint_reference(SourcePhysicsContactMemory&,
    std::uint32_t instance,std::uint32_t datum,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const NativeTu15ConstraintReferenceReleaseServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintReferenceReleaseStats* = nullptr);
} // namespace halo2
