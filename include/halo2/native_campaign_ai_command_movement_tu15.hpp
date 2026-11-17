#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
// Already-evaluated cs_go_to argument (2B0C19..2B0C45). Uses the actual
// 50EEE8 command context and 50EEE0 datum pool. HS thread yield/lifetime is
// owned by the VM bridge; this publishes no unit position or control result.
void native_campaign_ai_command_go_to_tu15(SourcePhysicsContactMemory &,
    std::uint32_t point,const SourcePhysicsContactRecordObserver & = {});
// Ground, nonswarm command kinds 1 and 3: 262A00 initialization and 263140 update
// and 263AA0 cleanup take actor, unit, actor-command-state pointer, command
// datum (RET10, AX; 263140 returns full EAX). Kind3 validates its second
// point, normalizes the authored facing direction, and waits for the actual
// actor orientation before completion.
// 263BD0 takes ECX actor/EAX command (RET, AL); 264860 validates EDX point
// (RET, AL). Explicit child refusals remain on auxiliary aim/swarm branches.
// Existing native publication owners perform the actual goal/path update.
void native_campaign_ai_command_movement_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
