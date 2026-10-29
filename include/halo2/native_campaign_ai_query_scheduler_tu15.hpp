#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Generic actor/query memberships; all addresses refer to actual retained
// engine storage. These owners do not cancel a behavior through its callback.
std::int16_t native_campaign_ai_query_find_slot_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t query);
void native_campaign_ai_query_release_slot_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::int16_t slot,const SourcePhysicsContactRecordObserver & = {});
std::int16_t native_campaign_ai_query_select_slot_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::int16_t priority,const SourcePhysicsContactRecordObserver & = {});
bool native_campaign_ai_query_reserve_member_tu15(SourcePhysicsContactMemory &,
    std::uint32_t query,std::uint32_t actor,std::int16_t priority,
    std::uint32_t expiration_bits,const SourcePhysicsContactRecordObserver & = {});
// Broadcast request: kind WORD +0, query datum DWORD +4, expiration DWORD +8.
void native_campaign_ai_query_broadcast_tu15(SourcePhysicsContactMemory &,
    std::uint32_t request,std::uint32_t excluded_actor,
    const SourcePhysicsContactRecordObserver & = {});
struct NativeCampaignAiQueryBroadcastRequest {
    std::uint16_t kind;
    std::uint32_t query,expiration_bits;
};
// The actual 278460 caller owns these three fields in its private stack local.
// This overload takes their native typed owner, disjoint from mutable engine
// storage, so a native caller needs no reconstructed source stack.
void native_campaign_ai_query_broadcast_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignAiQueryBroadcastRequest &,std::uint32_t excluded_actor,
    const SourcePhysicsContactRecordObserver & = {});
// Behavior state: query datum DWORD +10, member index WORD +18. The actual
// behavior cancellation callback must already have run before this generic
// retirement owner. The original shared delete requires caller DF=0.
void native_campaign_ai_query_retire_tu15(SourcePhysicsContactMemory &,
    std::uint32_t behavior_state,const SourcePhysicsContactRecordObserver & = {});
}
