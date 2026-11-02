#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
// Authenticated virtual34 feature retrieval:2E5E20 swept wrapper,3079C0 SoA,
//3067D0 box,3070F0 capsule,1F6690 polygon and2E8D40 sphere. Feature IDs/count/output are
// actual DATA; there is no saved-register/control-frame storage interface.
// The source sphere's REP MOVSD requires the ordinary clear-DF call ABI.
void native_tu15_contact_shape_features(SourcePhysicsContactMemory&,
    NativeTu15ContactComponent,NativeTu15ContactVirtualOwner,
    std::uint32_t feature_ids,std::uint32_t count,std::uint32_t output,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
