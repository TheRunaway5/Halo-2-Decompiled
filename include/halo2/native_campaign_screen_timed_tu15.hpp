#pragma once
#include "halo2/physics_contact_record.hpp"
#include <optional>

namespace halo2 {
struct NativeCampaignScreenTimedFpTu15 {
    std::uint16_t control_word=0x27f,status{};
    std::uint32_t mxcsr=0x1f80;
};
class NativeCampaignScreenDofArgumentsTu15 {
public:
    virtual ~NativeCampaignScreenDofArgumentsTu15()=default;
    // The six original stack arguments, read at the actual semantic phases.
    virtual std::uint32_t word(unsigned index)const=0;
};
// Complete native1FC10: focus clamp and the two authored DOF transition
// endpoints. FP policy is explicit; storage and arguments stay lazy.
void native_campaign_screen_dof_setup_tu15(SourcePhysicsContactMemory &,
    std::uint32_t focus_bits,const NativeCampaignScreenDofArgumentsTu15 &,
    NativeCampaignScreenTimedFpTu15 &,const SourcePhysicsContactRecordObserver & = {});
// Complete native1FD00: update all authored timed screen controllers and
// publish their outputs/capture request. An inactive interface has no result;
// an active one returns its actual retained address.
std::optional<std::uint32_t> native_campaign_screen_timed_update_tu15(
    SourcePhysicsContactMemory &,NativeCampaignScreenTimedFpTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
}
