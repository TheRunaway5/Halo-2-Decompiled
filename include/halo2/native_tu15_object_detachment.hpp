#pragma once
#include "halo2/native_tu15_object_deletion.hpp"
namespace halo2 {
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
};
// ActualC0F60. Preserve byte admission, signed16 descriptor fields/logical
// count shift, cached current-object lifetime, current descriptor re-reads,
// child recursion and tail sibling traversal. Genuine unknown children refuse.
void native_tu15_detach_object_tree(SourcePhysicsContactMemory&,
    NativeTu15ObjectDetachmentRequest,NativeTu15PhysicsFrameFp&,
    const NativeTu15ObjectDetachmentServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectDetachmentStats* = nullptr);
}
