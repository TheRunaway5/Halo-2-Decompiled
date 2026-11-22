#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// The identity is an aligned, otherwise unowned 64 KiB DATA range. The live
// bank checks its current owner index. Fixture extents include complete
// allocations/reservations and any outer private owner.
class NativeCampaignAiBipedRefreshScratchBindingTu15 {
public:
    static constexpr std::uint32_t bytes=0x10000u;
    NativeCampaignAiBipedRefreshScratchBindingTu15(SourceRuntimeMemory &memory,std::uint32_t identity)
        :NativeCampaignAiBipedRefreshScratchBindingTu15(memory,identity,{},
            [](const SourcePhysicsContactMemory &owner,std::uint32_t first) {
                const auto &bank=static_cast<const SourceRuntimeMemory&>(owner);
                return bank.find_unowned_range(first,std::uint64_t(first)+bytes,bytes,16u)==first;
            }) {}
    NativeCampaignAiBipedRefreshScratchBindingTu15(SourcePhysicsContactMemory &,
        std::uint32_t identity,std::span<const NativeTu15ContactPrivateRange> retained);
    ~NativeCampaignAiBipedRefreshScratchBindingTu15();
    NativeCampaignAiBipedRefreshScratchBindingTu15(const NativeCampaignAiBipedRefreshScratchBindingTu15 &)=delete;
    NativeCampaignAiBipedRefreshScratchBindingTu15 &operator=(const NativeCampaignAiBipedRefreshScratchBindingTu15 &)=delete;
private:
    friend void native_campaign_ai_biped_refresh_call_tu15(SourcePhysicsContactMemory &,
        std::uint32_t,SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver &);
    using GapCheck=bool (*)(const SourcePhysicsContactMemory &,std::uint32_t);
    NativeCampaignAiBipedRefreshScratchBindingTu15(SourcePhysicsContactMemory &,
        std::uint32_t identity,std::span<const NativeTu15ContactPrivateRange> retained,GapCheck);
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
