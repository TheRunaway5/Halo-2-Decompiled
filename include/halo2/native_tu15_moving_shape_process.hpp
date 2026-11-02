#pragma once
#include "halo2/native_tu15_contact_query.hpp"

namespace halo2 {
// The sole retained arithmetic output of the parent is its 64-byte relative
// transform, consumed synchronously by the cache initializer. The caller owns
// this aligned DATA interval; saved registers and return storage are excluded.
struct NativeTu15MovingShapeProcessScratch {std::uint32_t relative_transform{};};
struct NativeTu15MovingShapeProcessServices {
    std::function<std::uint64_t()> timestamp_counter;
    std::function<std::uint32_t(NativeTu15ContactComponent,
        NativeTu15ContactVirtualOwner,NativeTu15PhysicsFrameFp&)> classify_shape;
    std::function<void(std::uint32_t owner,std::uint32_t cache,
        NativeTu15ContactComponent first,NativeTu15ContactComponent second,
        std::uint32_t relative_transform,NativeTu15PhysicsFrameFp&)> initialize_cache;
    // Actual2F8B60, thiscall4 / RET16. No contact outcome is supplied here.
    std::function<void(NativeTu15ContactChild,NativeTu15ContactPayload first,
        NativeTu15ContactPayload second,NativeTu15ContactContext,
        std::uint32_t contact_output,NativeTu15PhysicsFrameFp&)> process_convex;
};
struct NativeTu15MovingShapeProcessStats {
    std::uint64_t calls{},completed{},refusals{},unsupported_fp{},
        classifications{},eligibility_tests{},motion_vectors{},cleared_vectors{},
        cache_refreshes{},convex_processes{},profile_entries{};
};
// Actual2E5320. Source current DATA, packed four-lane SIMD and native x87
// comparison are preserved. The qualified incoming-depth domain is explicit;
// deeper stacks and unmasked arithmetic must be admitted by separate proof.
void native_tu15_process_moving_shape_contact(SourcePhysicsContactMemory&,
    NativeTu15ContactChild,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15ContactContext,
    std::uint32_t contact_output,NativeTu15MovingShapeProcessScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15MovingShapeProcessServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15MovingShapeProcessStats* = nullptr);
} // namespace halo2
