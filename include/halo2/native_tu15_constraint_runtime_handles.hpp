#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15ConstraintRuntimeHandleServices {
    // ActualC6120 receives these two ordinary fields and literal scalar0.
    std::function<void(std::uint32_t reference,std::uint32_t context,
        std::uint32_t scalar_bits,NativeTu15PhysicsFrameFp&)> publish_zero_tagged_value;
    // Actual17A4A0; the17A0F0 caller does not consume its AL result.
    std::function<void(std::uint32_t handle,NativeTu15PhysicsFrameFp&)> destroy_handle;
};
struct NativeTu15ConstraintRuntimeHandleStats {
    std::uint64_t calls{},none_skips{},index_skips{},salt_skips{},valid_handles{},links_marked{},
        tag_cleanup_calls{},definition_skips{},tag_rows{},tag_fields{},tag_value_calls{},
        unbound_tag_value_calls{},retained_marks{},destroy_calls{},unbound_destroy_calls{},
        clear_calls{},clear_links{},clear_handles{},presence_calls{};
};
// Actual17A0F0/17A030. Ordinary lifetime fields only; missing child ownership
// counts and throws, with no original17A0F0 replay.
void native_tu15_release_constraint_runtime_handle(SourcePhysicsContactMemory&,
    std::uint32_t handle,bool deletion_requested,NativeTu15PhysicsFrameFp&,
    const NativeTu15ConstraintRuntimeHandleServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintRuntimeHandleStats* = nullptr);
// Actual230A60/230AE0, preserving independent link/handle gates and lazy loads.
void native_tu15_clear_constraint_runtime_handles(SourcePhysicsContactMemory&,
    std::uint32_t instance,bool clear_links,bool clear_handles,NativeTu15PhysicsFrameFp&,
    const NativeTu15ConstraintRuntimeHandleServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintRuntimeHandleStats* = nullptr);
bool native_tu15_constraint_has_runtime_handles(const SourcePhysicsContactMemory&,
    std::uint32_t instance,NativeTu15ConstraintRuntimeHandleStats* = nullptr);
} // namespace halo2
