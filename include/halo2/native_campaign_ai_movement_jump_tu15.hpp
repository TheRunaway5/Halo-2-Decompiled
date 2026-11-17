#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
// Actual FF1A0 calls from200420: EAX target, ECX origin, ESI direction output.
// Stack maximum speed, gravity1, minimum-speed output,0,0,highArc1,0,0,0,
// vertical-velocity output,horizontal-speed output. AL and saved GP/ESP/FP
// are the consumed contract; volatile registers/private frame are not claimed.
void native_campaign_ai_movement_jump_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
