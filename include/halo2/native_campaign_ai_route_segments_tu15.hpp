#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
// Native route segment reduction. 2CBEE0 takes six stack arguments:
// request, signed WORD input count, input rows, WORD count output, output
// rows, completion byte. It returns with RET 18 and has no defined result.
// Rows are 1C bytes; the real aligned caller frame is retained because the
// traversal and corner owners consume pointers into it. Reference callbacks
// remain available only for explicitly selected offline reference modes.
// Also owns 2CCEB0 (six stack arguments, AL, RET18), 2CC5C0 (EAX point,
// ECX center, EDX output, XMM6 radius, stack side, RET4), 2CC790 (EAX
// center, ECX tangents, EDX output, XMM7 radius, stack origin, RET4), and
// 2CC8F0 (EAX previous, ESI target, EBX/EDI corners, stack current, AL,
// RET4). Requires masked FP exceptions and an initially empty x87 stack.
// Whole-call storage/order/FP parity is qualified; faulted FST/FSTP
// continuations and exceptional CRT acos arguments are not admitted here.
void native_campaign_ai_route_segments_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry, SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
