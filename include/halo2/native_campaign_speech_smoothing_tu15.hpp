#pragma once
#include "halo2/native_campaign_audio_leaves.hpp"
#include "halo2/physics_contact_record.hpp"
#include <optional>
#include <exception>
#include <stdexcept>

namespace halo2 {
// Semantic original TU1.5 18F7A0. The caller supplies the real object datum
// and sampled scalar; original source memory owns admission and both reads
// of the retained field.
struct NativeCampaignSpeechSmoothingDeltaTu15 {
    std::uint32_t bits;
    bool selected_bound;
};
// Completed semantic quantities, including a partially completed update when
// real source storage faults. The thin binding publishes owed scalar lanes.
struct NativeCampaignSpeechSmoothingProgressTu15 {
    std::optional<std::uint32_t> lower_bound,difference,upper_bound;
    std::optional<NativeCampaignSpeechSmoothingDeltaTu15> selected_delta;
    std::optional<std::uint32_t> current_level,updated_level;
};
class NativeCampaignSpeechSmoothingPendingErrorTu15 final:public std::runtime_error {
    NativeCampaignSpeechSmoothingProgressTu15 progress_;NativeCampaignAudioFp fp_;std::exception_ptr cause_;
public:
    NativeCampaignSpeechSmoothingPendingErrorTu15(NativeCampaignSpeechSmoothingProgressTu15 progress,NativeCampaignAudioFp fp,std::exception_ptr cause):
        std::runtime_error("Speech smoothing has a partially completed level update"),
        progress_(std::move(progress)),fp_(fp),cause_(std::move(cause)){}
    const NativeCampaignSpeechSmoothingProgressTu15 &progress()const noexcept{return progress_;}
    NativeCampaignAudioFp floating_point()const noexcept{return fp_;}
    std::exception_ptr cause()const noexcept{return cause_;}
};
NativeCampaignSpeechSmoothingProgressTu15 native_campaign_speech_smoothing_tu15(SourcePhysicsContactMemory &,std::uint32_t object,
    std::uint32_t sampled_bits,NativeCampaignAudioFp &,
    const SourcePhysicsContactRecordObserver & = {});
}
