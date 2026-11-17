#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <array>
#include <functional>
#include <stdexcept>

namespace halo2 {
class NativeCampaignAiResourceProtectionTu15;
// A callee of the 1BBEE0 closure that has no native replication yet. The
// native path stops at the actual call, after every preceding source read and
// store; it never runs, skips or substitutes the original body.
class NativeCampaignAiPerformRequiredOwnerTu15 final:public std::runtime_error {
    std::uint32_t site_,entry_;
public:
    NativeCampaignAiPerformRequiredOwnerTu15(std::uint32_t site,std::uint32_t entry);
    std::uint32_t site()const noexcept{return site_;}
    std::uint32_t entry()const noexcept{return entry_;}
};
// Transport for a callee without a native owner. The kernel has published the
// arguments and the actual return address at abi.gp[4] and the original
// register inputs; the callee must leave its original exit ABI. Production
// passes none, which is the named refusal above. Only HALO2_REFERENCE_*
// oracles may supply the translated body here.
using NativeCampaignAiPerformCalleeTu15=
    std::function<void(std::uint32_t site,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &)>;
// Services the native closure needs from its live owner.
struct NativeCampaignAiPerformServicesTu15 {
    // Real page owner for the 1D2D90 -> 1498E0 -> 130030 allocator retirement.
    const NativeCampaignAiResourceProtectionTu15 *protection{};
    NativeCampaignAiPerformCalleeTu15 callee;
    // Reference differential only: route 26B220 through callee instead of its
    // native type-4 owner, so live state can be compared against the body.
    bool reference_candidate_preparation{};
    // Reference differential only: run the native type-4 26B220 slice, restore
    // the request, run callee's translated body and require identical request
    // bytes, FP controls and stack. Matched calls are counted here.
    std::uint64_t *differential_candidate_preparation{};
    // Reference only: route 26BA30 through callee instead of its native owner.
    bool reference_candidate_collection{};
    // Reference differential only: run native 26BA30, restore the candidate
    // array and request, run callee's translated body and require identical
    // array/request bytes, EAX, FP controls and stack. Matched calls are
    // counted here.
    std::uint64_t *differential_candidate_collection{};
    // Reference only: route all of 26A380 through callee (its native owner
    // is a prefix that refuses at the first unported point).
    bool reference_goal_evaluation{};
    // Reference differential only: run the native 26A380 prefix, restore its
    // stores, run callee's translated body; when the native completed (no
    // candidates) require identical stores and EAX. [0] matched, [1] native
    // refusals (the translated result is kept).
    std::array<std::uint64_t,2> *differential_goal_evaluation{};
};
// Existing native26A2E0 goal-selection owner exposed for other actual callers.
// EDI actor, EBX type-4 request; stack mode, output actor, leased workspace,
// output flag. RET10. FP is published on normal and exceptional returns.
void native_campaign_ai_goal_selection_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Original 1BBEE0, the "perform" callback of behavior descriptor 48A9D8 (row
// 109, slot +40). abi is the actual entry state: [esp] return, [esp+4] actor,
// [esp+8] behavior state, live FP controls. On return it holds the original
// exit: AL result, callee-saved registers, ESP after RET 8. The frame, the
// 0x758-byte type-4 request and every argument packet are published to the
// real stack in original order. Natively owned callees: 200C00, 26C510,
// 1BBBD0, 1D2D90, 1D2E30, the 26A2E0 root with 26B220 (type 4), 26BA30 and
// the 26A380 prefix (complete without candidates) and 26C3C0
// (native_campaign_ai_publication_tu15.hpp). Unported callees, including
// 26A380's 2013B0 path query (its remainder after 2013B0 is native), are
// named refusals; services.callee serves only references.
void native_campaign_ai_area_perform_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Original 1BC0A0 (descriptor 48A9D8 slot +48): publish the actor's +4AE and
// +484 flags and, with a live +26C reference, the 456914 scalar at +4B4.
// abi: [esp+4] actor; exit is RET 8 with the original EAX/ECX/EDX/XMM0.
void native_campaign_ai_area_perform_release_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver & = {});
}
