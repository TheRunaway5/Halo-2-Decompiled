#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
struct NativeCampaignAiSpeechResolvedSound {std::uint32_t value=0xffffffffu;};
// Original140EA0, including unsigned input and its minimum width of one.
std::uint32_t native_campaign_ai_sound_history_width_tu15(std::uint32_t count);
// Original21FE40: real class flag, sound/range metadata and history bit ordinal.
std::uint32_t native_campaign_ai_sound_history_prefix_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t range,std::uint32_t definition);
// Original213890: inspect the five live eligibility bits after range history.
bool native_campaign_ai_sound_permutation_blocked_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t sound);
// Original116B80. The output is dereferenced unconditionally, and written only
// when its incoming DWORD is nonzero. The returned boolean is independent of
// that publication rule; no provider admission is supplied.
bool native_campaign_ai_speech_source_tu15(
    SourcePhysicsContactMemory &,std::uint32_t unit,std::int16_t action,
    std::uint32_t resolved_sound_address);
bool native_campaign_ai_speech_source_tu15(
    SourcePhysicsContactMemory &,std::uint32_t unit,std::int16_t action,
    NativeCampaignAiSpeechResolvedSound &resolved_sound);
// Original116C00. Raw pointers are real optional engine-address operands and
// may alias unit state or each other. The typed view is the actual117240 local
// output contract; no source-address stack is constructed for that overload.
bool native_campaign_ai_speech_reserve_tu15(
    SourcePhysicsContactMemory &,std::uint32_t unit,std::int16_t action,
    std::int16_t priority,std::uint32_t prior_reference_address,
    std::uint32_t resolved_sound_address);
bool native_campaign_ai_speech_reserve_tu15(
    SourcePhysicsContactMemory &,std::uint32_t unit,std::int16_t action,
    std::int16_t priority,NativeCampaignAiSpeechResolvedSound &resolved_sound,
    std::uint32_t *prior_reference=nullptr);
}
