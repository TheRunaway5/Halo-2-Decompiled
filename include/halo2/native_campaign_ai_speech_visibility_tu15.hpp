#pragma once
#include "halo2/native_campaign_audio_leaves.hpp"
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Whole11E660: actual cluster->environment->tag bit. No visibility or policy
// result is supplied when a real owning record is absent.
bool native_campaign_ai_speech_location_environment_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t location);
bool native_campaign_ai_speech_cluster_environment_tu15(
    const SourcePhysicsContactMemory &,std::int16_t cluster);
// Whole26DBE0 with genuine character policy, authored radius, actual point
// geometry, environment bits, cluster blockage/scale and source FP controls.
// Return is AX16, with the original0/2/3 outcomes. One available extended
// temporary is needed only after the source gates reach the square root.
std::uint16_t native_campaign_ai_speech_visibility_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor,
    std::uint32_t target,std::uint32_t observer,std::uint32_t target_location,
    std::int16_t alert,std::int16_t mode,NativeCampaignAudioFp &,
    unsigned available_extended_temporaries);
// The genuine203CD0 caller's private point snapshot and query XYZ are ordinary
// native values; retained policy/location/BSP/tag owners stay real and lazy.
std::uint16_t native_campaign_ai_speech_visibility_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t actor,
    std::span<const std::uint32_t,3> target,
    std::span<const std::uint32_t,14> observer,std::uint32_t target_location,
    std::int16_t alert,std::int16_t mode,NativeCampaignAudioFp &,
    unsigned available_extended_temporaries);
}
