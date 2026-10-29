#pragma once
#include "halo2/native_campaign_audio_leaves.hpp"
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
enum class NativeCampaignAiSpeechUnitChannelPhase {untouched,blend,immediate};
struct NativeCampaignAiSpeechUnitChannelEffect {
    NativeCampaignAiSpeechUnitChannelPhase phase=NativeCampaignAiSpeechUnitChannelPhase::untouched;
    std::uint32_t scalar_bits{},scaled_rate_bits{};
};
// Original1110D0. Unit state and indexed byte publications remain lazy, with
// the source's signed channel domain (including its unusual invalid indices).
NativeCampaignAiSpeechUnitChannelEffect native_campaign_ai_speech_unit_channel_tu15(
    SourcePhysicsContactMemory &,std::uint32_t unit,std::int16_t channel,
    std::uint32_t target_bits,std::uint32_t duration_bits,NativeCampaignAudioFp &);
struct NativeCampaignAiSpeechActorChannelEffect {
    bool published{};
    std::uint32_t target_bits{};
    NativeCampaignAiSpeechUnitChannelEffect unit;
};
// Complete215B40: actual actor->unit channel update and actor+6CE WORD timer.
NativeCampaignAiSpeechActorChannelEffect native_campaign_ai_speech_actor_channel_tu15(
    SourcePhysicsContactMemory &,std::uint32_t actor,std::int16_t channel,
    std::uint16_t timer,NativeCampaignAudioFp &);
struct NativeCampaignAiSpeechTickConversion {
    std::uint32_t ticks{},scaled_seconds_bits{};
};
// Complete1494F0, actual signed live tick rate and original SSE->FISTP rounding.
// One real extended temporary is required. No clock value is supplied.
NativeCampaignAiSpeechTickConversion native_campaign_ai_speech_seconds_to_ticks_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t seconds_bits,
    NativeCampaignAudioFp &,unsigned available_extended_temporaries);
}
