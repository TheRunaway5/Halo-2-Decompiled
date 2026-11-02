#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"
namespace halo2 {
// Complete actual3061F0/306220. The latter's supplied child-output buffer is
// unused by this genuine list owner; no synthetic shape or pointer is created.
std::uint32_t native_tu15_collection_child_count(const SourcePhysicsContactMemory&,
    NativeTu15ContactChild);
NativeTu15ContactChild native_tu15_collection_child(const SourcePhysicsContactMemory&,
    NativeTu15ContactChild,std::uint32_t index);
struct NativeTu15CollectionFilterServices {
    std::function<NativeTu15ContactChild(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        std::uint32_t index,std::uint32_t output,NativeTu15PhysicsFrameFp&)> resolve_child;
    std::function<std::uint32_t(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> classify_shape;
    std::function<std::uint32_t(std::uint32_t motion,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> motion_type;
};
struct NativeTu15CollectionFilterScratch {
    //256 aligned caller-owned bytes for the actual virtual child resolver.
    std::uint32_t child_shape{};
};
// Complete1CBE70, including185020's authenticated shape-class predicate and
//2E98D0's original group matrix. The ordinary context input is unread by the
// source. All classifications/resolution/motion calls keep their actual owners.
std::uint32_t native_tu15_filter_collection_child(SourcePhysicsContactMemory&,
    std::uint32_t receiver,std::uint32_t output,NativeTu15ContactPayload second,
    NativeTu15ContactPayload first,NativeTu15ContactChild collection,std::uint32_t index,
    NativeTu15CollectionFilterScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15CollectionFilterServices&,
    const SourcePhysicsContactRecordObserver& = {});
}
