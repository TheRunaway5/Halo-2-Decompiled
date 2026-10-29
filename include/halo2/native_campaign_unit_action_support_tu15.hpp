#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
struct NativeCampaignUnitActionArgumentsTu15 {
    // Caller input reads stay lazy: type is read after the tick store, result
    // after the type store. No action/behavior outcome is manufactured here.
    std::function<std::uint16_t()> type;
    std::function<std::uint8_t()> result;
};
struct NativeCampaignUnitInterruptCallTu15 {std::uint32_t callback,unit,type;};
using NativeCampaignUnitInterruptObserverTu15=std::function<void(const NativeCampaignUnitInterruptCallTu15&)>;
void native_campaign_unit_mark_update_tu15(SourcePhysicsContactMemory &,std::uint32_t unit);
// The required1D/1C/1F/20 interruption families close natively. NULL callback
// metadata is honored; actual8FDB0 is empty. Other callback identities fail.
void native_campaign_unit_interrupt_action_tu15(SourcePhysicsContactMemory &,
    std::uint32_t unit,std::uint32_t type,const NativeCampaignUnitInterruptObserverTu15 & = {});
void native_campaign_unit_record_action_tu15(SourcePhysicsContactMemory &,
    std::uint32_t history,const NativeCampaignUnitActionArgumentsTu15 &);
void native_campaign_unit_notify_action_tu15(SourcePhysicsContactMemory &,
    std::uint32_t player,const NativeCampaignUnitActionArgumentsTu15 &);
}
