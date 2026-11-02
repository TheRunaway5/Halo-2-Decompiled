#pragma once
#include "halo2/native_tu15_constraint_reference_release.hpp"
namespace halo2 {
struct NativeTu15ModelConstraintRemovalServices {
    // Actual17A0F0 consumes its genuine handle and literal deletion flag.
    std::function<void(std::uint32_t handle,bool deletion_requested,
        NativeTu15PhysicsFrameFp&)> release_runtime_handle;
    NativeTu15ConstraintReferenceReleaseServices reference;
};
struct NativeTu15ModelConstraintRemovalStats {
    std::uint64_t calls{},cached_slots_cleared{},links_marked{},handle_releases{},
        unbound_handle_releases{},local_rows{},local_references{},global_rows{},
        global_references{},runtime_removals{},unbound_runtime_removals{},
        list_rows{},list_removals{},list_deletions{},instance_deletions{};
    NativeTu15ConstraintReferenceReleaseStats reference;
    NativeTu15ConstraintBodySlotStats slots;
    NativeTu15ConstraintScalarCacheStats cache;
};
// Actual22D0A0. Removal operates on the genuine instance datum and its actual
// owning component/lists. No native admission can replay the original parent.
void native_tu15_remove_model_constraint(SourcePhysicsContactMemory&,
    std::uint32_t constraint_datum,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const NativeTu15ModelConstraintRemovalServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ModelConstraintRemovalStats* = nullptr);
} // namespace halo2
