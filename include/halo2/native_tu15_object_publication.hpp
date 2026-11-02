#pragma once
#include "halo2/native_tu15_activation_services.hpp"
namespace halo2 {
struct NativeTu15ObjectPublicationStats {
    std::uint64_t calls{},root_queries{},tree_calls{},ancestor_wakes{},marked_headers{};
};
// Actual BCFB0 and BE7E0 over current authored object-header/data ownership.
std::uint32_t native_tu15_publication_root(const SourcePhysicsContactMemory&,
    NativeTu15PhysicsObject);
void native_tu15_mark_publication_tree(SourcePhysicsContactMemory&,
    NativeTu15PhysicsObject,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectPublicationStats* = nullptr);
// Actual BDA50. Cached header-bank identities and source reread order survive
// recursive/observer changes; no guest calling convention enters the kernel.
void native_tu15_publish_object_update(SourcePhysicsContactMemory&,
    NativeTu15PhysicsObject,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectPublicationStats* = nullptr);
} // namespace halo2
