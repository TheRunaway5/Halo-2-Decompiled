#pragma once
#include "halo2/native_tu15_convex_contact_query.hpp"
#include "halo2/native_tu15_agent_cleanup_children.hpp"
namespace halo2 {
struct NativeTu15ConvexProcessScratch {
    // Caller-owned aligned0x230B DATA backing, plus a disjoint4B scalar.
    // Contact64B, direction16B, plane16B, transform64B and simplex DATA
    // have independent source lifetimes. No saved/control bytes initialize it.
    std::uint32_t identity{},result_scalar{};
};
struct NativeTu15ConvexProcessServices {
    NativeTu15ConvexContactServices geometry;
    NativeTu15AgentCleanupChildrenServices cleanup;
    decltype(NativeTu15ContactProducerServices::publish_contact) publish_contact;
    decltype(NativeTu15ContactProducerServices::publish_distance) publish_distance;
};
struct NativeTu15ConvexProcessStats {
    std::uint64_t calls{},completed{},refusals{},fast_projections{},fast_separations{},
        simplex_calls{},contacts{},distance_publications{},direction_updates{},
        cache_removals{},profile_entries{};
};
// Actual2F8B60. The4 formal DATA arguments and current receiver replace no
// GP/SIMD context. Remaining simplex/publication/listener services are explicit;
// this typed semantic parent is not a production-native-closure claim.
void native_tu15_process_convex_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15ContactContext,NativeTu15ContactOutput,
    NativeTu15ConvexProcessScratch,NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ConvexProcessServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConvexProcessStats* = nullptr);
} // namespace halo2
