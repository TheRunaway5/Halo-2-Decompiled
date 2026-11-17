#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
// Native obstacle corridor search and its candidate/heap/tangent owners.
// 2CDEB0: EAX output, ECX target sector, EDX start sector, ESI target XY;
// stack nav, request, option byte, obstacles, radius, start XY, last, retry.
// AL success and RET20. Retains the real child frames for native traversal.
void native_campaign_ai_route_corridor_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry, SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
