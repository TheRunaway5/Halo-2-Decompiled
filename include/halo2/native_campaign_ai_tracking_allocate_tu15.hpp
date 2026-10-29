#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/native_tu15_data_array_iteration.hpp"
namespace halo2 {
// Actual1EC970 caller-private actor iterator; storage is disjoint from retained
// engine pools. The previous datum is published even on exhaustion.
struct NativeCampaignAiActorIteratorTu15 {
    NativeTu15DataArrayIterator storage;
    bool active_only;
    std::uint32_t previous;
};
std::uint32_t native_campaign_ai_actor_next_tu15(
    SourcePhysicsContactMemory &,NativeCampaignAiActorIteratorTu15 &);
//2677D0: retire every actual tracking query in an actor's property list.
std::uint16_t native_campaign_ai_actor_tracking_retire_tu15(
    SourcePhysicsContactMemory &,std::uint32_t actor);
//266150: initialize the actual query extension, preserving lazy default reads.
void native_campaign_ai_tracking_extension_reset_tu15(
    SourcePhysicsContactMemory &,std::uint32_t extension);
//267870: actual three-pass pressure policy; returns its signed WORD count>0.
bool native_campaign_ai_tracking_pressure_tu15(SourcePhysicsContactMemory &,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
//266280: choose/retire an actual cache slot, allocate a real datum, initialize
// its query/extension and publish links. Inputs are genuine caller datums.
std::uint32_t native_campaign_ai_tracking_allocate_tu15(
    SourcePhysicsContactMemory &,std::uint32_t actor,std::uint32_t perception,
    std::int16_t priority,NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
}
