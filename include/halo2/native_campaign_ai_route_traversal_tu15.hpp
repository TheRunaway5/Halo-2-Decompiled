#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
// Native navigation edge iteration, segment contact and sector traversal.
// Actual entry frames; native host arithmetic under the retained FP controls.
// 2761F0: EAX start, ECX end, ESI contact; stack nav, sector, target, exclusions.
// 2762A0: EAX sector, ESI contact; stack nav, start, target, direction, max, exclusions.
// 2769C0: fourteen stack arguments, RET38, AL intersection result.
void native_campaign_ai_route_traversal_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},const SourcePhysicsContactRecordObserver & = {});
}
