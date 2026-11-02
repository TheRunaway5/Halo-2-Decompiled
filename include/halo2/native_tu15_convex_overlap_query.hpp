#pragma once
#include "halo2/native_tu15_conditional_contact_process.hpp"
#include "halo2/native_tu15_convex_contact_query.hpp"
namespace halo2 {
struct NativeTu15ConvexOverlapQueryScratch {
    // Own16B plane,64B relative geometry, and the simplex DATA for the whole
    // actual solver call. Source plane+20,relative+30,simplex+70 offsets.
    std::uint32_t identity{};
};
struct NativeTu15ConvexOverlapQueryServices {
    NativeTu15ConvexContactServices geometry;
    // Actual collector virtual+4, thiscall2 / RET8.
    std::function<void(NativeTu15ContactQueryCollector,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsBody,NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&)> collect;
};
struct NativeTu15ConvexOverlapQueryStats {
    std::uint64_t calls{},completed{},refusals{},solver_calls{},collect_calls{},profile_entries{};
};
// Actual2F8410. Context (third formal) is unused by the source.
void native_tu15_query_convex_overlap(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
    NativeTu15ContactQueryCollector,NativeTu15ConvexOverlapQueryScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ConvexOverlapQueryServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConvexOverlapQueryStats* = nullptr);
// Authenticated41CA8C+4→30BA80. Exactly writes collector+4=1; both payload
// formals are unused. No other result or padding is initialized.
void native_tu15_mark_contact_query_hit(SourcePhysicsContactMemory&,
    NativeTu15ContactQueryCollector,const SourcePhysicsContactRecordObserver& = {});
// Actual8EB50: one RET16, no operand read/store or FP operation.
void native_tu15_ignore_contact_process() noexcept;
} // namespace halo2
