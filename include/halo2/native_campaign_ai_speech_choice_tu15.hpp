#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/native_campaign_audio_leaves.hpp"
namespace halo2 {
struct NativeCampaignAiSpeechCandidate {
    std::array<std::uint32_t,4> payload;
    std::uint32_t weight_bits;
};
static_assert(sizeof(NativeCampaignAiSpeechCandidate)==20);
struct NativeCampaignAiSpeechChoice {
    std::int16_t selected=-1;
    bool fp_touched{},inspected_score{};
    // Actual numerical outcomes, also used for thin caller-live SIMD result
    // transport. No architectural register file belongs to this algorithm.
    std::uint32_t cumulative_bits{},target_bits{},last_term_bits{};
};
// Whole217370: actual retained score array and actual4F3CD8 RNG owner. The
// original caller needs one available extended temporary except count1.
// Missing storage, unavailable scratch or unmasked FP fail explicitly.
NativeCampaignAiSpeechChoice native_campaign_ai_speech_choice_tu15(
    SourcePhysicsContactMemory &,std::uint32_t candidates,std::int16_t count,
    NativeCampaignAudioFp &,unsigned available_extended_temporaries,
    const SourcePhysicsContactRecordObserver & = {});
// Actual213D30's candidates are a private local buffer. This typed owner
// shares the same algorithm and needs no reconstructed source stack. RNG and
// literal inputs still come from their real retained owners.
NativeCampaignAiSpeechChoice native_campaign_ai_speech_choice_tu15(
    SourcePhysicsContactMemory &,std::span<NativeCampaignAiSpeechCandidate>,
    std::int16_t count,NativeCampaignAudioFp &,unsigned available_extended_temporaries,
    const SourcePhysicsContactRecordObserver & = {});
}
