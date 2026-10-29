#pragma once
#include "halo2/native_campaign_audio_leaves.hpp"
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete252F30, actual structure visibility bits or the legacy cluster byte.
bool native_campaign_ai_speech_cluster_blocked_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t structure,
    std::int32_t first,std::int32_t second);
// Complete203C00 with its real502200 bitmap /502240 key owner. The native
// argument is the actual caller's private signed WORD value. All structure
// metadata and scale results come from their real retained owners.
std::uint32_t native_campaign_ai_speech_cluster_mask_tu15(
    SourcePhysicsContactMemory &,std::int16_t cluster,NativeCampaignAudioFp &);
}
