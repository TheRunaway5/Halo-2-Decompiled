#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
// Actual2CABE0 obstacle collection. Ten stack arguments, RET40; preserves
// EBX/EBP/ESI/EDI. The caller consumes the two obstacle lists, not volatile
// GP/SIMD scratch. Real spatial, policy and marker owners compute all inputs.
void native_campaign_ai_route_obstacles_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
}
