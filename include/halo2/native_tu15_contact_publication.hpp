#pragma once
#include "halo2/native_tu15_contact_reduction.hpp"
namespace halo2 {
struct NativeTu15ContactPublicationScratch {
    // Aligned0xB0 caller-owned DATA: two world points, initialized manager
    // packet32B, possible fifth48B record and five current record identities.
    std::uint32_t identity{};
};
struct NativeTu15ContactPublicationServices {
    NativeTu15ContactDistanceServices removal;
    // Actual current manager virtual4(first,second,context,packet32), RET16.
    // Return the genuine point16 identity; no recorded result is an input.
    std::function<std::uint16_t(std::uint32_t manager,NativeTu15ContactVirtualOwner,
        NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
        std::uint32_t packet,NativeTu15PhysicsFrameFp&)> create_point;
};
struct NativeTu15ContactPublicationStats {
    std::uint64_t attempts{},completed{},refusals{},matched_records{},point_requests{},
        created_points{},reductions{};
    NativeTu15ContactReductionStats reduction;
};
// Actual30CD00: match an existing cached contact or build and request a real
// point, then reduce a full cache with native30CAA0. Source two world-position
// transforms use the sole qualified native2E4690 owner. Private CPU control
// storage and child return images are never accepted.
void native_tu15_publish_contact(SourcePhysicsContactMemory&,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext,NativeTu15ContactRecord,std::uint32_t manager,
    std::uint32_t records,std::uint32_t count_field,NativeTu15ContactPublicationScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ContactPublicationServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactPublicationStats* = nullptr);
} // namespace halo2
