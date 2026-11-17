#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
struct NativeCampaignAiRoutePendingIntegerTu15 {std::array<std::byte,10> extended{};};
class NativeCampaignAiRoutePendingIntegerErrorTu15 final:public std::runtime_error {
    NativeCampaignAiRoutePendingIntegerTu15 pending_;
    std::exception_ptr cause_;
public:
    NativeCampaignAiRoutePendingIntegerErrorTu15(NativeCampaignAiRoutePendingIntegerTu15,std::exception_ptr);
    const auto &pending()const noexcept{return pending_;}
    const auto &cause()const noexcept{return cause_;}
};
// Native route materialization (27A640) and parent-chain reconstruction
// (27AAA0). Their pointer-consumed frames use the real caller's stack.
// 27A640: EAX workspace; stack actor, output; RET 8, AL result.
// 27AAA0: EBX workspace; stack actor, goal, node WORD, capacity WORD,
// rows, complete-byte output, pair-count WORD output, pairs, pair capacity
// WORD; RET 24, AX count. EBX/EBP/ESI/EDI and ESP follow the original ABI;
// caller-saved scratch registers are not an interface guarantee.
// Composes native paired-link endpoints (279100), segment reduction (2CBEE0)
// and smoothing (2CE030). Remaining dependencies refuse at their actual call
// after preceding effects: 27A8E6 -> 1B0E10 and special-link kinds2/5 ->
// 27A110. services.callee is for explicit reference modes only.
// Direct continuation entries 27A7A7 and 27A887 qualify the corresponding
// post-call segments from independently declared frame/input state. Their
// ESP is 27A640's frame, EBX workspace, EBP output (AL is the 2CE030 result
// at 27A887); they are not substituted callee outcomes in production.
void native_campaign_ai_route_call_tu15(SourcePhysicsContactMemory &,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &,const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
