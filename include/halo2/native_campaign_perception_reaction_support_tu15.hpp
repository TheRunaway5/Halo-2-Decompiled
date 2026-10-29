#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <optional>
namespace halo2 {
struct NativeCampaignPerceptionTransitionTu15 {
    bool accepted{};
    std::optional<std::uint16_t> sector;
};
NativeCampaignPerceptionTransitionTu15 native_campaign_perception_transition_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t location,std::uint32_t point,NativeTu15PhysicsFrameFp &);
bool native_campaign_perception_transition_tu15(SourcePhysicsContactMemory &,
    std::uint32_t location,std::uint32_t point,std::uint32_t sector_destination,NativeTu15PhysicsFrameFp &);
std::uint32_t native_campaign_perception_flagged_root_tu15(const SourcePhysicsContactMemory &,std::uint32_t object);
std::uint32_t native_campaign_perception_vehicle_root_tu15(const SourcePhysicsContactMemory &,std::uint32_t object);
std::uint32_t native_campaign_perception_parent_root_tu15(const SourcePhysicsContactMemory &,std::uint32_t object);
bool native_campaign_perception_definition_bit7_tu15(const SourcePhysicsContactMemory &,std::uint32_t object);
bool native_campaign_perception_root_state_tu15(const SourcePhysicsContactMemory &,std::uint32_t object);
bool native_campaign_perception_root_cacheable_tu15(const SourcePhysicsContactMemory &,std::uint32_t object);
}
