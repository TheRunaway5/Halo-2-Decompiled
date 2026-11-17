#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"

namespace halo2 {
// The identity is an aligned, otherwise unowned 64 KiB DATA range. Retained
// extents must include complete live allocations/reservations, including any
// outer private owner. The binding copies that metadata and owns no source
// instruction, result, or register callback.
class NativeCampaignAiBipedRefreshScratchBindingTu15 {
public:
    NativeCampaignAiBipedRefreshScratchBindingTu15(SourcePhysicsContactMemory &,
        std::uint32_t identity,std::span<const NativeTu15ContactPrivateRange> retained);
    ~NativeCampaignAiBipedRefreshScratchBindingTu15();
    NativeCampaignAiBipedRefreshScratchBindingTu15(const NativeCampaignAiBipedRefreshScratchBindingTu15 &)=delete;
    NativeCampaignAiBipedRefreshScratchBindingTu15 &operator=(const NativeCampaignAiBipedRefreshScratchBindingTu15 &)=delete;
private:
    friend void native_campaign_ai_biped_refresh_call_tu15(SourcePhysicsContactMemory &,
        std::uint32_t,SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver &);
    struct State;
    State *state_;
};
// Actual callee-entry ABI: E20F0 uses EDI biped and seven stack arguments,
// RET 1C. E1DD0 uses EAX optional origin, ECX direction, EBX instance and nine
// stack arguments, RET 24. 1FD8A0 uses EAX actor, ECX origin, EBX direction,
// plain RET. Volatile GP/XMM are not outputs; AL, saved GP, retained writes
// and floating-point controls/status are published. Collision children are
// composed only from native kernels, using the bound guarded DATA lifetime.
void native_campaign_ai_biped_refresh_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
}
