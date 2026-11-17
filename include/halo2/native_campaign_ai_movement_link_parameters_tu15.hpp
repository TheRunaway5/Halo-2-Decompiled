#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
//200310 selects the actor's authored jump speed: EAX actor datum, EDI optional
// one-byte force output; XMM0 REAL result.278FA0 reads scenario speed indexed by
// signed CX. Both preserve FP state and return with RET0. The inherited
// character lookup is the existing native1ECD00 owner.
void native_campaign_ai_movement_link_parameters_call_tu15(
    SourcePhysicsContactMemory &,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
