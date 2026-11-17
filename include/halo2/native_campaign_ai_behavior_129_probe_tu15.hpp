#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
//1CA900 selects the nearest eligible perception and a planar probe direction;
// stack actor, request, RET8/AL.1CAA20 builds eight directions from the actor
// facing; EDI actor, EBX request, RET0/AL. Both stop honestly at their actual
//1CA560 call while that owner is unavailable. No post-child loop is admitted.
// Saved GP/AL, retained DATA, FP, actual refusal GP/arguments/directions are
// defined; other volatile results and dead private frames are scratch.
void native_campaign_ai_behavior_129_probe_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
