#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// Actual retained tag-block identities; zero and inherited NONE are original
// decisions. Missing backing fails through the storage owner, never a default.
std::uint32_t native_campaign_ai_character_navigation_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t character);
std::uint32_t native_campaign_ai_character_vehicle_navigation_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor,std::uint32_t vehicle_character);
std::uint32_t native_campaign_ai_order_character_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t order);
std::uint32_t native_campaign_ai_actor_order_character_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor);
std::uint32_t native_campaign_ai_character_movement_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor);
}
