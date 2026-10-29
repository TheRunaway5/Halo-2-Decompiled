#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_campaign_unit_action_support_tu15_owns_entry(std::uint32_t);
// Routing admission for the proved1C/1D/1F/20 interrupt families. Other
// entry ABIs are complete; E8E10/E8E50 begin is deliberately unowned.
bool native_campaign_unit_action_support_tu15_owns_call(
    std::uint32_t,const SourcePhysicsWorldRuntimeAbi &);
void native_campaign_unit_action_support_tu15_run(SourcePhysicsContactMemory &,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsWorldRuntimeServices & = {});
struct NativeCampaignUnitActionSupportActivityTu15 {
    std::uint64_t marks{},interrupts{},notifications{},records{};
};
NativeCampaignUnitActionSupportActivityTu15 native_campaign_unit_action_support_activity_tu15();
}
