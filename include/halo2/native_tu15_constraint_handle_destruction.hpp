#pragma once
#include "halo2/native_tu15_constraint_tag_cleanup.hpp"
namespace halo2 {
struct NativeTu15ConstraintHandleDestructionServices {
    NativeTu15ConstraintRuntimeHandleServices tagged_values;
    // Actual177FA0's child datum/record and parent link fields are DATA.
    std::function<void(std::uint32_t child,std::uint32_t child_record,
        std::uint32_t parent_state,std::uint32_t parent_head,NativeTu15PhysicsFrameFp&)> unlink_child;
    // Actual177020 consumes one ordinary child datum.
    std::function<void(std::uint32_t child,NativeTu15PhysicsFrameFp&)> delete_child;
};
struct NativeTu15ConstraintHandleDestructionStats {
    std::uint64_t calls{},valid_handles{},invalid_handles{},object_queries{},object_references{},detaches{},
        detached_references{},links_marked{},immediate_deletions{},deferred_deletions{},
        event_lists{},event_deletions{},child_unlinks{},child_deletions{},
        unbound_child_unlinks{},unbound_child_deletions{},tail_pairs{},row_deletions{},deactivated_children{};
    NativeTu15ConstraintRuntimeHandleStats tagged_values;
};
// Actual17A4A0 with native17A420/BCDF0/17A380/17A230/17A2F0 and16E8A0.
// The boolean is the actual AL result. Remaining child-link owners count and
// throw without replaying an admitted parent.
bool native_tu15_destroy_constraint_runtime_handle(SourcePhysicsContactMemory&,
    std::uint32_t handle,NativeTu15PhysicsFrameFp&,
    const NativeTu15ConstraintHandleDestructionServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintHandleDestructionStats* = nullptr);
} // namespace halo2
