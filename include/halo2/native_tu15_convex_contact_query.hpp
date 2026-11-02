#pragma once
#include "halo2/native_tu15_contact_query.hpp"
namespace halo2 {
struct NativeTu15ConvexContactScratch {
    // Retained scalar/shape/plane/transform/simplex DATA starts at+14.
    // Existing bytes remain owned; callers must not zero-fill or inject results.
    std::uint32_t identity{},result_scalar{};
};
struct NativeTu15ConvexContactServices {
    // Actual30E930, with actual typed arguments and owned simplex/plane DATA.
    std::function<std::uint32_t(std::uint32_t simplex,
        NativeTu15ContactComponent first,NativeTu15ContactComponent second,
        std::uint32_t relative_transform,std::uint32_t support_cache,
        std::uint32_t plane,NativeTu15PhysicsFrameFp&)>solve_simplex;
    std::function<std::uint64_t()>timestamp_counter;
};
// Actual2E4760. Packed rotation includes the original W lane and X+Y+Z order.
void native_tu15_rotate_contact_direction(SourcePhysicsContactMemory&,
    std::uint32_t output,std::uint32_t basis,std::uint32_t source,
    NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {});
// Actual30D350. The cache argument is unused by the source algorithm.
void native_tu15_build_convex_contact_result(SourcePhysicsContactMemory&,
    std::uint32_t simplex,std::uint32_t plane,std::uint32_t output,
    std::uint32_t retained_scalar,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {});
// Complete semantic2F8630 parent, native2E4550/2E4760/30D350 and genuine
// remaining solver service. Private CPU control state is not an input/output.
std::uint32_t native_tu15_query_convex_contact(SourcePhysicsContactMemory&,
    NativeTu15ContactChild,std::uint32_t valid_result,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15ContactContext,std::uint32_t output,
    NativeTu15ConvexContactScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15ConvexContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
