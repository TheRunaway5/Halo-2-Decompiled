#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
struct NativeTu15ContactShapeScratch {
    std::uint32_t scalar{},candidate_dots{};
    // A sweep wrapper delegates to a separately owned child DATA lifetime.
    // This native ownership link is not an emulated control-frame chain.
    const NativeTu15ContactShapeScratch*child{};
};
// Actual2E5DA0 tail delegation,3077C0 SoA center,306750 box center and
//2E7E10 capsule endpoint center and2E8D70 zero sphere center.
// Owner identities authenticate the source algorithm; unknown owners refuse.
void native_tu15_contact_shape_center(SourcePhysicsContactMemory&,
    NativeTu15ContactComponent,NativeTu15ContactVirtualOwner,std::uint32_t output,
    const SourcePhysicsContactRecordObserver& = {});
// Actual1CB030. The source returns the all-ones vertex encoding sentinel.
std::uint32_t native_tu15_contact_shape_vertex_encoding()noexcept;
// Actual2E5DB0 sweep support,3077F0 SoA support,306790 box support,
//2E7D90 capsule endpoints,2E8D20 zero sphere support and1F6610 polygon support.
// Preserve source tie/NaN branches, real feature tags and all FP rounding.
void native_tu15_contact_shape_support(SourcePhysicsContactMemory&,
    NativeTu15ContactComponent,NativeTu15ContactVirtualOwner,std::uint32_t direction,
    std::uint32_t output,const NativeTu15ContactShapeScratch&,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
