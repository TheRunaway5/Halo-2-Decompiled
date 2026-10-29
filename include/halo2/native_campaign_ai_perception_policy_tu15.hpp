#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
//2676C0: an actual query extension has a target datum at +50.
bool native_campaign_ai_perception_has_target_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t perception);
//1ED240/1ED450: authored inherited character blocks used by tracking and alerts.
std::uint32_t native_campaign_ai_character_tracking_policy_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor);
std::uint32_t native_campaign_ai_character_perception_policy_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor);
//1EA1D0/1EA2E0: the real signed seat's vehicle datum and tag/unit flag policy.
std::uint32_t native_campaign_ai_actor_vehicle_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor);
bool native_campaign_ai_actor_vehicle_policy_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor);
//1ED530/1ED4F0: find the actual matching vehicle block through character parents.
std::uint32_t native_campaign_ai_character_vehicle_perception_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor,std::uint32_t vehicle);
std::uint32_t native_campaign_ai_actor_vehicle_perception_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor);
//27C7E0: positive signed byte override, otherwise the original signed WORD.
std::int16_t native_campaign_ai_order_perception_variant_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t policy);
struct NativeCampaignAiChildPlayerTu15 {std::uint32_t player,unit;};
//1C1D70's actual caller-private optional output profile. No retained output
// address is synthesized; both resulting datums are ordinary typed values.
NativeCampaignAiChildPlayerTu15 native_campaign_ai_child_player_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t parent);
}
