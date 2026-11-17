#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
// Native2CE030 corridor smoothing. Eleven stack arguments: actor, request,
// signed WORD row count, rows, policy byte, output WORD count, output rows,
// completion byte, optional object output, WORD object-kind output, optional
// final-obstacle byte. AL result; RET2C; preserved EBX/EBP/ESI/EDI. Pointer
// consumed scratch is retained at the actual1F88-byte probed caller frame.
void native_campaign_ai_route_smoothing_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},const SourcePhysicsContactRecordObserver & = {});
}
