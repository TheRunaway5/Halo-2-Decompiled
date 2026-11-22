#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
// 275BA0 follows the object's parent chain and returns navigation eligibility
// in AL (object datum in EAX, plain RET). 275CB0 takes the object in ECX,
// [ESP+4] optional index output and [ESP+8] framed-point output, RET 8.
// The latter owns the parent lookup, biped cached point and empty-scenario
// paths. E2300's dynamic-sector check E04B0, cache reset E22C0 and salted
// object validation share their actual caller frames. E20F0 collision refresh,
// 202660 sector lookup and 2185E0 moving-frame lookup use their existing
// navigation owners. F35D0 vehicle point requires a call owner, and
// scenario projection 276E10 remains an explicit required owner.
void native_campaign_ai_object_navigation_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {},
    const NativeCampaignAiPerformCalleeTu15 &vehicle_point = {});
}
