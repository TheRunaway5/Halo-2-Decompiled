#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <functional>

namespace halo2 {
// Arguments of original 26A380 (RET 20), the 26A2E0 goal evaluation.
struct NativeCampaignAiGoalEvaluationTu15 {
    std::uint32_t actor{},request{},candidates{},count{},candidate_out{},owner_out{},workspace{},flag_out{};
};
// 2013B0, the type-4 path query (another owner): EBX = the 0x50-byte query
// in 26A380's frame, [esp+4] = actor, RET 4. stack is ESP at 2013B0's first
// instruction. It must leave the query (and any frame bytes it owns) in
// retained memory, as the original does.
using NativeCampaignAiGoalPathQueryTu15=std::function<void(SourcePhysicsContactMemory &,std::uint32_t actor,
    std::uint32_t query,std::uint32_t stack,NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver &)>;
struct NativeCampaignAiGoalEvaluationContextTu15 {
    std::uint32_t stack{};          // ESP at 26A380's first instruction ([stack] = return)
    NativeTu15PhysicsFrameFp *fp{}; // live caller controls, threaded through
    unsigned occupied_x87{};        // the caller's live x87 values
    // Empty: the named refusal 26A70D -> 2013B0. Set it to the native
    // 2013B0 owner once it exists; nothing else changes.
    NativeCampaignAiGoalPathQueryTu15 path_query;
};
// Native 26A380. Without candidates (count low word 0) it clears *flag_out,
// raises [5020A8]+3, resets the actor's goal history and returns NONE.
// With candidates and a type-4 shape (request +618/+54/+55/+56 clear) it
// runs the 2013B0 path query through context.path_query, then natively:
// the 26A712 query fields, 201500 navigation filter, 27B010 workspace,
// 27B2B0 path search (*flag_out = 1 when found), the candidate distances
// with 27A460 path costs, the +5E8 retry count, either the 269E70 random
// pick or the 269D70 scoring, 140910/269DF0 sort and 45687C evaluators,
// then the chosen goal, *candidate_out and *owner_out (140DB0).
// Named refusals (NativeCampaignAiPerformRequiredOwnerTu15):
//   26A70D -> 2013B0 without a path query owner;
//   26A438 -> 26A43E request +618; 26A5B5 -> 26A5B7 request +54;
//   26A628 -> 26A62E request +55;  26A6FB -> 26A846 request +56;
//   26AD7E -> 2694C0 and 26AEC8 -> 2013B0 (unreachable for type 4);
//   and those of native_campaign_ai_goal_scoring_tu15.hpp.
// Retained reads/stores are published in original order. The frame's
// private locals are host values; the query (ESP+44), the 201500 output
// (ESP+94) and the sort indices (ESP+B0) are real frame bytes, because
// their consumers read them from memory. Returns the original EAX.
std::uint32_t native_campaign_ai_evaluate_goals_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignAiGoalEvaluationTu15 &,const NativeCampaignAiGoalEvaluationContextTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
// Prefix-only form (no stack, FP or path query): identical stores, and it
// refuses at 26A70D -> 2013B0 whenever candidates exist.
std::uint32_t native_campaign_ai_evaluate_goals_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignAiGoalEvaluationTu15 &,const SourcePhysicsContactRecordObserver & = {});
// Actual 26A380 ABI: [esp] return, [esp+4..20] the eight arguments. Exit:
// EAX = the goal, ESP after RET 20, EBX/EBP/ESI/EDI preserved. Without
// candidates also EDX = NONE, ECX = 0 and XMM0 = 0; otherwise ECX, EDX and
// XMM are caller-saved scratch (26A2E0/1BBEE0 never read them).
void native_campaign_ai_evaluate_goals_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
// The original continuation at 26A712, right after 2013B0 returned: ESP is
// 26A380's entry ESP - 0x8B0, EBP the request, and the frame holds the
// prologue's locals and the query. For isolated original comparison while
// 2013B0 has no native owner. Exit as above; EBX/ESI/EDI/EBP are the frame's
// saved words.
void native_campaign_ai_evaluate_goals_resume_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
}
