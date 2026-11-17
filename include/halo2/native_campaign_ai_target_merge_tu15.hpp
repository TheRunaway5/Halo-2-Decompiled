#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
//273290 merges tracked targets into a destination clump. EAX destination
// datum, ECX source datum; RET0. Retained DATA, ordered stores, saved GP and FP
// follow source. No scalar result or dead volatile/private-frame guarantee.
void native_campaign_ai_target_merge_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
