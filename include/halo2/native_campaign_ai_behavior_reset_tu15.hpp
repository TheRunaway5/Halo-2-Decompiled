#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
class NativeCampaignAiBehaviorResetArgumentsTu15 {
public:
    virtual ~NativeCampaignAiBehaviorResetArgumentsTu15()=default;
    virtual std::uint32_t actor()const=0;
    virtual std::uint32_t action()const=0;
};
// Native1C9CE0: reset the actor's local behavior markers and the selected
// action record, then report successful initialization. Argument/record reads
// remain lazy; all publications retain original order and scalar widths.
bool native_campaign_ai_behavior_reset_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignAiBehaviorResetArgumentsTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
}
