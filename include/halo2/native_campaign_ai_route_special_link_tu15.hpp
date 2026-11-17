#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
//279100 materializes the two framed endpoints of a special navigation link.
// EAX navigation, ECX link index, EDI first endpoint; stack actor, request,
// previous framed point, previous sector, first-sector output, second framed
// endpoint, second-sector output. RET1C; AL and saved GP are defined outputs.
// Retained DATA, ordered publications and FP controls/status follow source;
// dead volatile registers and private frame bytes are not an ABI guarantee.
void native_campaign_ai_route_special_link_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
