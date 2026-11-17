#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
// 200420: EAX actor datum, ECX optional authored link-mask byte pointer;
// stack destination XYZ pointer and force byte (upper argument bits ignored).
// AL result, RET8, callee-saved GP and retained FP. Calculates the actual
// ballistic request before publishing actor movement fields. The unit-mode5
// action request remains a named E8E50 dependency until its real owner exists.
void native_campaign_ai_movement_link_transition_call_tu15(
    SourcePhysicsContactMemory &,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
