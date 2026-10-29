#pragma once
#include "halo2/native_campaign_level_transition_tu15.hpp"
namespace halo2 {
// Complete32C0A0 last-character search over an actual retained C string.
// The source's forward scan, backward scan and final byte reread stay lazy.
std::uint32_t native_campaign_string_last_character_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t text,std::uint8_t character);
//1A5B60: first path record with an identical case-sensitive basename after
// the last backslash. Forward slashes have no special source meaning.
std::uint32_t native_campaign_scenario_record_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t scenario_path);
// Completed-value counterpart over the actual decoded native catalog.
const NativeCampaignLevelPathTu15*native_campaign_scenario_record_tu15(
    std::span<const NativeCampaignLevelPathTu15>,std::string_view scenario_path);
}
