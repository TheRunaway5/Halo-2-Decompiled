#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Original 26BA30, firing-position candidate collection for the 26A2E0 goal
// selection. Arguments are the actual stack words: actor datum, the caller's
// 0x78-byte candidate array, its capacity and the type-4 request (may be 0).
// Returns the original EAX: the dword candidate count.
//
// Natively replicated closure: 26BBD0 (squad/order/zone walk, recursive
// order redirect), 26BE90 (one area's firing positions), 26C0B0 (admission,
// including the 26C740 -> 140DB0 owner hash lookup with its 267A30/267A40
// callbacks and the 3EA20 x87 owner/self distances), 26C1E0 (candidate
// initialization with the 218B20 frame projection), 2A78A0/202AB0 (cluster
// list at request+0x6A0), 26C850/277D40 (goal definitions).
//
// Every retained read and store keeps the original order and width; x87 and
// SIMD arithmetic run the original instruction sequence under the caller's
// controls, so status/MXCSR sticky flags are the original's. The callees'
// frames below the caller's ESP are private and dead on return; their locals
// are host values, not reconstructed stack bytes.
//
// Named refusals (std::runtime_error, never a substituted result):
//  - a hash table at [56AEB0] whose +2C/+30 callbacks are not 267A30/267A40
//    (NativeCampaignAiPerformRequiredOwnerTu15 at 140DB8/140DD9);
//  - original stack corruption: a 26C740 value size above 4, a 2A78A0 cluster
//    index whose unbounded mask word reaches its saved EBP or return address
//    (clusters below -160 or above 63), a zero hash bucket count (#DE);
//  - 26BBD0 order redirects deeper than 4096 (the original overflows its stack).
std::uint32_t native_campaign_ai_collect_candidates_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t candidates,std::uint32_t capacity,std::uint32_t request,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
// Actual 26BA30 ABI: [esp] return, [esp+4..10] the four arguments, live FP
// controls with at most four live x87 values. Exit: EAX count, ESP after
// RET 10, EBX/EBP/ESI/EDI preserved. ECX/EDX are dead at the only caller
// (26A2E0 overwrites both before use) and are left unchanged.
void native_campaign_ai_collect_candidates_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
}
