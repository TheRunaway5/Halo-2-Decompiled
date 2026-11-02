#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
struct NativeTu15ContactQueryBody {std::uint32_t identity{};};
// Retained DATA used by actual2E4FE0: byte results, cache state and the64-byte
// relative transform. The source reuses portions of this data for later record
// construction. Existing bytes must remain owned and must not be zero-filled.
struct NativeTu15ContactQueryScratch {std::uint32_t identity{};};
struct NativeTu15ContactQueryServices {
    std::function<std::uint32_t(NativeTu15ContactComponent,
        NativeTu15ContactVirtualOwner,NativeTu15PhysicsFrameFp&)>classify_shape;
    std::function<void(std::uint32_t owner,std::uint32_t cache,
        NativeTu15ContactComponent first,NativeTu15ContactComponent second,
        std::uint32_t relative_transform,NativeTu15PhysicsFrameFp&)>initialize_cache; //309060/309150
    std::function<void(NativeTu15ContactChild,std::uint32_t valid_result,
        NativeTu15ContactPayload first,NativeTu15ContactPayload second,
        NativeTu15ContactContext,std::uint32_t contact_output,
        NativeTu15PhysicsFrameFp&)>query_convex; //2F8630
};
struct NativeTu15ContactQueryStats {
    std::uint64_t calls{},classifications{},eligibility_tests{},motion_vectors{},
        cache_refreshes{},convex_queries{};
};
// Actual283380. Reads the source2001 body property and genuine component/unit
// pools, then the source parent-membership list. No allocation or FP effects.
bool native_tu15_body_contact_eligible(SourcePhysicsContactMemory&,
    NativeTu15ContactQueryBody);
// Complete actual2E4FE0 semantic parent. Returns the actual caller byte-result
// identity; remaining services execute genuine owners and supply no recorded
// result or guest GP/SIMD postimage. Caller-private CPU storage is not modeled.
std::uint32_t native_tu15_query_contact(SourcePhysicsContactMemory&,
    NativeTu15ContactChild,std::uint32_t valid_result,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext,std::uint32_t contact_output,
    NativeTu15ContactQueryScratch,NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ContactQueryServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactQueryStats* = nullptr);
} // namespace halo2
