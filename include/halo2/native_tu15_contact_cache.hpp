#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
// Caller-owned vector DATA used by the cache algorithms. Identity is aligned;
// the interval+10..+80 contains centers, support vertices and directions. No
// saved CPU registers or return identities are part of this storage contract.
struct NativeTu15ContactCacheScratch {std::uint32_t identity{};};
struct NativeTu15ContactCacheServices {
    std::function<void(NativeTu15ContactComponent,NativeTu15ContactVirtualOwner,
        std::uint32_t output,NativeTu15PhysicsFrameFp&)>shape_center;
    std::function<void(NativeTu15ContactComponent,NativeTu15ContactVirtualOwner,
        std::uint32_t direction,std::uint32_t output,
        NativeTu15PhysicsFrameFp&)>support_vertex;
    std::function<std::uint32_t(NativeTu15ContactComponent,
        NativeTu15ContactVirtualOwner,NativeTu15PhysicsFrameFp&)>vertex_encoding;
};
// Actual309060: initialize the two-shape support cache using the real shape
// centers, original point/direction transforms and native SIMD operations.
void native_tu15_initialize_convex_contact_cache(SourcePhysicsContactMemory&,
    std::uint32_t cache,NativeTu15ContactComponent first,
    NativeTu15ContactComponent second,std::uint32_t relative_transform,
    NativeTu15ContactCacheScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactCacheServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual309150: three authored triangle vertices transformed into first-shape
// space, original normal orientation and actual first-shape support owner.
void native_tu15_initialize_triangle_contact_cache(SourcePhysicsContactMemory&,
    std::uint32_t cache,NativeTu15ContactComponent first,
    NativeTu15ContactComponent triangle,std::uint32_t relative_transform,
    NativeTu15ContactCacheScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactCacheServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
