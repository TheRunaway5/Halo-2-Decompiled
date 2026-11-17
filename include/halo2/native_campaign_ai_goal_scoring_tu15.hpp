#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Native callees of 26A380's candidate ranking (the 0x78-byte firing-position
// candidates built by 26BA30). Every retained read and store keeps the
// original order, width and PC; SSE and x87 arithmetic run the original
// instruction sequences under the caller's MXCSR and control word.
//
// Callback tables are data. Only the entries reachable from a type-4
// request have native owners; any other entry is a named refusal
// (NativeCampaignAiPerformRequiredOwnerTu15 at the indirect CALL):
//   45681C (269D70): 267A60 (all types) and 2689A0 (bit 4);
//   45687C (269DB0 and 26A380's request loop): 269870 (all types).
// Further named refusals: 269870's visibility test 2698FF -> 2A7D60,
// 267A60's swept-volume test 267D53 -> 120AF0, 269E70's 269EE2 -> 2694C0
// (request +618), and 140910 with a comparator other than 269DF0.

// 26C5A0 (EAX actor, [esp+4] goal; RET 4): goal in the actor's four goal
// history entries (+402, stride 6)?
bool native_campaign_ai_goal_history_contains_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t goal);
// 1209C0 (EAX segment vector, ECX origin, EDX point; plain RET): squared
// distance from the point to the clamped segment, returned in XMM0.
std::uint32_t native_campaign_ai_segment_distance_tu15(SourcePhysicsContactMemory &,
    std::uint32_t vector,std::uint32_t origin,std::uint32_t point,NativeTu15PhysicsFrameFp &);
// 269D70 (EDI request, EBX candidates, [esp+4] actor, [esp+8] count; RET 8).
void native_campaign_ai_score_candidates_tu15(SourcePhysicsContactMemory &,std::uint32_t actor,
    std::uint32_t request,std::uint32_t count,std::uint32_t candidates,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// 45687C evaluator walk with the original AL threading: 269DB0 passes one
// candidate; 26A380's 26ACCE loop passes candidate 0. Returns the final AL.
bool native_campaign_ai_evaluate_candidate_tu15(SourcePhysicsContactMemory &,std::uint32_t actor,
    std::uint32_t request,std::uint32_t candidate,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {},bool request_loop=false);
// 140910 quicksort with the 140BC0 selection sort for runs of at most 8,
// comparator 269DF0 (its [52D878] candidate base is read per comparison).
// The dword index array lives in retained memory; every element access is
// published with its original PC.
void native_campaign_ai_sort_candidates_tu15(SourcePhysicsContactMemory &,std::uint32_t array,
    std::uint32_t count,std::uint32_t comparator,std::uint32_t context,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// 269E70 (EAX candidate, ECX request, ESI actor; plain RET): random-pick
// admission of one candidate; returns AL (the candidate's final +4C).
bool native_campaign_ai_admit_candidate_tu15(SourcePhysicsContactMemory &,std::uint32_t actor,
    std::uint32_t request,std::uint32_t candidate,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual call ABI of each owner above, for direct original comparison:
// entry is the original function address (26C5A0, 1209C0, 269D70, 267A60,
// 2689A0, 269870, 269DB0, 140910, 269E70). Exit: AL result where the owner
// returns one (XMM0 for 1209C0), ESP after its RET, EBX/EBP/ESI/EDI
// preserved. ECX/EDX are caller-saved scratch and left unchanged.
void native_campaign_ai_goal_scoring_call_tu15(SourcePhysicsContactMemory &,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
}
