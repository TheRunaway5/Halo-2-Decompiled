#pragma once
#include "halo2/native_tu15_body_constraint_update.hpp"
namespace halo2 {
struct NativeTu15SupportCarrierScratch {
    // Caller-owned ordinary DATA, disjoint from the public inputs/outputs.
    // These are three52-byte affine values and two aligned16-byte packets.
    std::uint32_t frame{},inverse{},product{},point{},velocity{};
};
struct NativeTu15SupportCarrierRequest {
    std::uint32_t component_datum{},previous_velocity{},point{},previous_frame{},
        current_frame{},velocity{},delta{},basis{};
};
struct NativeTu15SupportCarrierStats {
    std::uint64_t calls{},missing_components{},invalid_body_slots{},point_queries{},
        disabled_bodies{},unknown_motion_owners{},completed{};
    NativeTu15ModelConstraintUpdateStats frames;
};
// Actual145560 retained0x34 affine product; preserves source alias/store order.
void native_tu15_support_affine_product(SourcePhysicsContactMemory&,std::uint32_t first,
    std::uint32_t second,std::uint32_t destination,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual184EC0, including1D8B90/144090/145560/1D8CD0/323A10.
// Output storage stays live and lazy; no returned CPU state is supplied.
bool native_tu15_update_support_carrier(SourcePhysicsContactMemory&,
    const NativeTu15SupportCarrierRequest&,NativeTu15SupportCarrierScratch,
    NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SupportCarrierStats* = nullptr);
}
