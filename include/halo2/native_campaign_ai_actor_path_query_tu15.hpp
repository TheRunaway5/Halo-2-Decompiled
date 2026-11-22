#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// 2013B0: EBX is the retained 0x50-byte query, [ESP+4] the actor, RET 4.
// 275E90: EDX is the actor, plain RET; populate its +27C framed point once.
// Actual stack words, including the uninitialized high half read at2014A7,
// remain in caller memory. Uncached 275CB0/275BA0 are native; their
// The vehicle branch follows the caller-provided F35D0 geometry owner.
void native_campaign_ai_actor_path_query_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {},
    const NativeCampaignAiPerformCalleeTu15 &vehicle_point = {});

// Actual 26A380 ABI with the native 2013B0 owner bound. The query receives
// the real caller frame and saved EBP/ESI/EDI words; caller-saved scratch
// is not part of the goal-result interface.
void native_campaign_ai_evaluate_goals_with_query_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});

// Shared native1D2CF0 aim-origin owner; the body is owned by area perform.
void native_campaign_ai_object_aim_origin_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t out,std::uint32_t stack,
    std::uint32_t saved_ebx,std::uint32_t saved_edi,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
}
