#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <stdexcept>

namespace halo2 {
class NativeCampaignScreenFlashRequiredTu15 final:public std::runtime_error {
    std::uint32_t instruction_,target_;
public:
    NativeCampaignScreenFlashRequiredTu15(std::uint32_t instruction,std::uint32_t target);
    std::uint32_t instruction()const noexcept{return instruction_;}
    std::uint32_t target()const noexcept{return target_;}
};
// Actual156E40 global fade and local-user flash sampling. Native hardware
// SIMD/x87 math retains explicit control/status; source storage remains lazy.
// The real CRT method selector admits its finite and zero fmod owners only;
// other selected handlers retain their exact required-owner boundary.
void native_campaign_screen_flash_sample_tu15(SourcePhysicsContactMemory &,
    std::uint32_t user,std::uint32_t destination,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
}
