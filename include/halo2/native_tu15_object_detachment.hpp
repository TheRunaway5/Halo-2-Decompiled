#pragma once
#include "halo2/native_tu15_object_deletion.hpp"
namespace halo2 {
struct NativeTu15SpatialRemovalStats {
    std::uint64_t calls{},outer_rows{},inner_rows{},outer_deletions{},inner_deletions{},
        head_clears{},attachment_calls{},inactive_attachments{},attachment_flag_clears{};
};
struct NativeTu15SpatialRemovalParameters {
    std::function<std::uint32_t()> key,head_pointer;
};
// Actual1D30F0 and C5250. The head pointer is ordinary scalar DATA; its lazy
// form preserves the source's final parameter re-read after nonempty chains.
// Data-array retirement uses the shared native16E8A0 algorithm with DF=0.
void native_tu15_remove_spatial_chain(SourcePhysicsContactMemory&,std::uint32_t root,
    const NativeTu15SpatialRemovalParameters&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15SpatialRemovalStats* = nullptr);
void native_tu15_remove_spatial_chain(SourcePhysicsContactMemory&,std::uint32_t root,
    std::uint32_t key,const std::function<std::uint32_t()>&head_pointer,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15SpatialRemovalStats* = nullptr);
void native_tu15_remove_spatial_chain(SourcePhysicsContactMemory&,std::uint32_t root,
    std::uint32_t key,std::uint32_t head_pointer,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15SpatialRemovalStats* = nullptr);
void native_tu15_detach_light_attachment(SourcePhysicsContactMemory&,std::uint32_t datum,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15SpatialRemovalStats* = nullptr);
struct NativeTu15ObjectDetachmentRequest {
    std::uint32_t datum{},detach_attachments{},delete_attachments{},follow_siblings{},current_only{};
};
struct NativeTu15ObjectDetachmentServices {
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> detach_attachment; //C5250
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> delete_attachment; //C4D30
    std::function<void(std::uint32_t datum,std::uint32_t detach,std::uint32_t deleting,
        NativeTu15PhysicsFrameFp&)> detach_children; //C3750
};
struct NativeTu15ObjectDetachmentStats {
    std::uint64_t calls{},objects{},guard_skips{},descriptor_queries{},attachment_detaches{},
        attachment_deletes{},children_detaches{},recursions{},sibling_steps{},unbound_services{};
    NativeTu15ObjectDeletionStats ancestor;
    NativeTu15SpatialRemovalStats attachments;
};
// ActualC0F60. Preserve byte admission, signed16 descriptor fields/logical
// count shift, cached current-object lifetime, current descriptor re-reads,
// child recursion and tail sibling traversal. Genuine unknown children refuse.
void native_tu15_detach_object_tree(SourcePhysicsContactMemory&,
    NativeTu15ObjectDetachmentRequest,NativeTu15PhysicsFrameFp&,
    const NativeTu15ObjectDetachmentServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectDetachmentStats* = nullptr);
}
