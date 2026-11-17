#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
//1CACE0, behavior129 descriptor+40. Stack actor,state; AL1,RET8. Owns real
// goal selection, query and path search, retaining actual caller frames and
// workspace leases. Native1CA900/1CAA20 probes retain their real unowned
//1CA560 boundary; no probe outcome is transported.
// Normal forward-string ABI, empty x87 stack and masked FP exceptions.
void native_campaign_ai_behavior_129_perform_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
