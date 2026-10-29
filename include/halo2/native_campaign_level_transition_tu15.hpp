#pragma once
#include "halo2/physics_contact_record.hpp"
#include <span>
#include <string_view>
namespace halo2 {
// Actual campaign/map/path data retained by the native asset catalog.
struct NativeCampaignLevelPathTu15 {
    std::int32_t campaign,map;
    std::string_view scenario_path;
};
//1A5C80: smallest signed map key greater than current, within the campaign.
std::int32_t native_campaign_next_level_key_tu15(
    const SourcePhysicsContactMemory &,std::int32_t campaign,std::int32_t current);
//1A5AB0: first matching real matg path record; returns null on absence.
std::uint32_t native_campaign_level_path_record_tu15(
    const SourcePhysicsContactMemory &,std::int32_t campaign,std::int32_t map);
// Same selection algorithms over the actual already-decoded native catalog.
// This completed-value seam creates no source tags, private stack or options.
std::int32_t native_campaign_next_level_key_tu15(
    std::span<const NativeCampaignLevelPathTu15>,std::int32_t campaign,std::int32_t current);
const NativeCampaignLevelPathTu15 *native_campaign_level_path_tu15(
    std::span<const NativeCampaignLevelPathTu15>,std::int32_t campaign,std::int32_t map);
}
