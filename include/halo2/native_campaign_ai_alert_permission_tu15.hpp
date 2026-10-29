#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <stdexcept>
namespace halo2 {
struct NativeCampaignAiAlertRequiredOwnerTu15:std::runtime_error {
    std::uint32_t site,target;
    std::uint16_t event;
    std::uint32_t speaker_unit,target_object;
    NativeCampaignAiAlertRequiredOwnerTu15(std::uint32_t call,std::uint32_t owner,
        std::uint32_t speaker,std::uint32_t object)
        :std::runtime_error("Native campaign alert requires its real speech owner"),site(call),target(owner),
         event(0xae),speaker_unit(speaker),target_object(object){}
};
//265620: actual selected-target alert permission, with genuine inherited
// character probability and25980 retained RNG publication. A reached213D30
// submission stops at26568A after the original preceding writes/reads.
void native_campaign_ai_alert_permission_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t perception,NativeTu15PhysicsFrameFp &,
    unsigned occupied_x87);
}
