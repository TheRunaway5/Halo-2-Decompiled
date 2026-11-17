#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"

namespace halo2 {
// Native 26C3C0, 1BBEE0's goal publication (CALL at 1BC054), with the
// movement owners it reaches: 1FC540 (action reset), 1FC720 (command point), 1FC9B0 (begin in mode
// 4/5/6), 200D30 (movement and path update), 200960, 200A10, E9600 and
// 27B0D0. Already-native callees are composed: 26C510, 26C600, 26C850,
// 16E940, 1ECC40/1ECD00, 218D00, 2013B0, 201500, 1D2D90/1D2E30,
// 27B010, 27B2B0 and 27A640/27AAA0 route reconstruction.
//
// Every other callee is a named NativeCampaignAiPerformRequiredOwnerTu15
// refusal at its actual CALL, after every preceding source read and store:
//   200D95 -> 1EBBD0 (+504 == 2 resume), 201022 -> 201840 (mode 6 test),
//   201058 -> 275FE0 (mode 7), 201127 -> 201750 (path mode 2),
//   201178 -> BCFB0 and 2011A5 -> 1D2CF0 (path mode 1, before 2A8440),
//   20128C -> 27A2D0; the query's uncached navigation boundaries and the
//   route's 279100/1B0E10 boundaries remain named refusals.
// services.callee serves only HALO2_REFERENCE_* oracles; production passes
// none. Native frames are published on the real guest stack at the original
// addresses wherever a callee or a later read consumes them.
//
// 26C3C0 entry: [esp] return, [esp+4] actor, [esp+8] goal, [esp+C] the
// goal's owner actor (or NONE), [esp+10] path workspace lease, [esp+14]
// path-found byte, [esp+18] begin byte. Exit: EAX = actor +418, EBX/EBP/
// ESI/EDI preserved, ESP += 1C. ECX/EDX/XMM are caller-saved scratch (no
// caller reads them before writing).
void native_campaign_ai_publication_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
// The same closure entered at 26C3C0, 1FC540 ([esp+4] actor, RET 4),
// 1FC720 (EBX actor, [esp+4] framed point, [esp+8] navigation reference,
// [esp+C] object, [esp+10] flag byte, RET10; AL),
// 1FC9B0 ([esp+4] actor, [esp+8] mode, [esp+C] goal, [esp+10] workspace,
// [esp+14] flag byte, RET 14) or 200D30 ([esp+4] actor, [esp+8] workspace,
// RET 8), for direct qualification. The last three return AL. Entries
// 20124A, 2012DF and 201342 resume 200D30 after 2013B0, 27B2B0 and a route
// owner from a declared post-call state (ESP its frame, EBP the actor row,
// ESI the actor, EDI the lease, AL the search result) and finish with its
// RET 8: isolated component qualification alongside the actual native
// query and route caller paths.
void native_campaign_ai_publication_call_tu15(SourcePhysicsContactMemory &,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
