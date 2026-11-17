#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
// 1EBBD0: ECX actor, XMM0.low REAL radius, RET. 1FB4F0: EAX actor,
// stack REAL radius, AL reached, RET4. The arrival test consumes actual
// actor points/path progress and unit velocity; it does not move a unit.
// 200A40: EAX actor, stack action-request byte, RET4. Ordinary command
// modes are native; navigation-link/action children refuse by source name.
// 218EB0/218F60 expose their native difference calculations for proof.
void native_campaign_ai_movement_resume_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
