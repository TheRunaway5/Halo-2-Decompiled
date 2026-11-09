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
struct NativeTu15SphereCapsuleOverlapQueryScratch {
    // Query scalar/profile word+C, closest point+10 and two transformed
    // capsule endpoints+20/+30. closest_point owns3128C0 scalar cells+8/+C;
    // collector owns the reversed wrapper's12-byte current pair collector.
    std::uint32_t identity{},closest_point{},collector{};
};
// Actual2FA9A0, ordinary sphere-first/capsule-second descriptors. The source
// transforms the actual capsule endpoints, projects the sphere centre onto
// their bounded segment, and emits a pair only for strict radius overlap.
void native_tu15_query_sphere_capsule_overlap(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
    NativeTu15ContactQueryCollector,NativeTu15SphereCapsuleOverlapQueryScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ConvexOverlapQueryServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConvexOverlapQueryStats* = nullptr);
// Actual2FAB70 is a cdecl4 wrapper, not a virtual thiscall/RET16 owner. It
// swaps the two descriptors through a current41CA40 reversed pair collector.
void native_tu15_query_reversed_sphere_capsule_overlap(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
    NativeTu15ContactQueryCollector,NativeTu15SphereCapsuleOverlapQueryScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ConvexOverlapQueryServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConvexOverlapQueryStats* = nullptr);
// Actual2F8410. Context (third formal) is unused by the source.
void native_tu15_query_convex_overlap(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
    NativeTu15ContactQueryCollector,NativeTu15ConvexOverlapQueryScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ConvexOverlapQueryServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConvexOverlapQueryStats* = nullptr);
// Actual2F9CA0. The receiver and third formal are unused. The source compares
// squared centre distance with squared summed radii, then calls collector+4
// only for strict overlap. Scratch owns its retained scalar/profile word+C.
// The typed input carries FP control/status only; require empty live x87.
void native_tu15_query_sphere_overlap(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody first, NativeTu15PhysicsBody second,
    NativeTu15ContactQueryCollector, NativeTu15ConvexOverlapQueryScratch,
    NativeTu15PhysicsFrameFp&, unsigned incoming_x87_depth,
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
