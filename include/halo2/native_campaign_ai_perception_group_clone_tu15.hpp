#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
//274570: EAX source group record, EBX destination clump record; returns the
// actual allocated group datum/NONE in EAX, RET0. Copies the source before
// allocation/retirement, then composes the existing native274200/274020 owners.
//2744E0: EAX source record, EDX destination; ordered payload copy, RET0.
// Retained DATA, result, callee-saved GP and FP are owned. The typed children
// do not expose unconsumed volatile registers or their private CPU frames.
// Both entries require the engine ABI's forward string direction (DF0).
void native_campaign_ai_perception_group_clone_call_tu15(
    SourcePhysicsContactMemory &,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
