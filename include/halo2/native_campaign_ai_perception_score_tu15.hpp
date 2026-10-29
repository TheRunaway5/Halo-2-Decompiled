#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <stdexcept>
namespace halo2 {
struct NativeCampaignAiPerceptionScoreRequiredOwnerTu15:std::runtime_error {
    std::uint32_t site,target;
    NativeCampaignAiPerceptionScoreRequiredOwnerTu15(std::uint32_t call,std::uint32_t owner)
        :std::runtime_error("Native perception score requires original multiplayer policy160A60 at26FE94"),site(call),target(owner){}
};
// Whole26FA40 returns the actual scalar XMM0 bits through normal typed data.
// Campaign mode1 uses its genuine bitset policy. Reached mode2/160A60 remains
// an explicit required-owner boundary, never a supplied team-relation result.
std::uint32_t native_campaign_ai_perception_score_tu15(
    SourcePhysicsContactMemory &,std::uint32_t actor,std::uint32_t perception,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
}
