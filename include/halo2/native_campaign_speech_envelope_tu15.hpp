#pragma once
#include "halo2/native_campaign_audio_leaves.hpp"
#include "halo2/physics_contact_record.hpp"
#include <exception>
#include <stdexcept>

namespace halo2 {
struct NativeCampaignSpeechEnvelopeSampleTu15 {
    bool present;std::uint32_t bits;
};
// A rejected scale read leaves the real, exactly converted byte sample as
// its pending scalar. This carries native mathematical data, not a CPU frame.
class NativeCampaignSpeechEnvelopePendingErrorTu15 final:public std::runtime_error {
    std::uint32_t sample_;
    std::exception_ptr cause_;
public:
    NativeCampaignSpeechEnvelopePendingErrorTu15(std::uint32_t,std::exception_ptr);
    std::uint32_t converted_sample_bits()const noexcept{return sample_;}
    const auto &cause()const noexcept{return cause_;}
};
// Complete21F860: resolve the actual sound/range/permutation resource, reject
// a nonmatching permutation, clamp the sample ordinal and normalize its byte.
// No resource readiness, voice or sound-admission result is supplied here.
NativeCampaignSpeechEnvelopeSampleTu15 native_campaign_speech_envelope_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t sound,std::int16_t range,
    std::int16_t permutation,std::int16_t sample,NativeCampaignAudioFp &);
}
