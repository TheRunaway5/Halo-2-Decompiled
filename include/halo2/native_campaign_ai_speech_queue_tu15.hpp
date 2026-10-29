#pragma once
#include "halo2/native_campaign_audio_leaves.hpp"
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Actual217870/217970 timing arithmetic leaves the caller's extended operands
// intact. These values describe only real scalar results for ABI transport.
struct NativeCampaignAiSpeechQueueMath {
    bool delay_computed{}, separation_loaded{};
    std::uint32_t delay_bits{}, separation_bits{};
};
struct NativeCampaignAiSpeechQueueAdmission {
    bool admitted{};
    NativeCampaignAiSpeechQueueMath math;
};
// Original217870: postpone candidate within the actual blocking speech window.
NativeCampaignAiSpeechQueueMath native_campaign_ai_speech_queue_postpone_tu15(
    SourcePhysicsContactMemory &,std::uint32_t candidate,std::uint32_t blocker,
    NativeCampaignAudioFp &,unsigned available_extended_temporaries);
// Original218070: mark this row and its already cancelled dependants.
void native_campaign_ai_speech_queue_cancel_tu15(
    SourcePhysicsContactMemory &,std::uint32_t datum);
// Original2180F0: unlink cancelled rows, clear actual unit references and free
// the real salted pool allocations. Caller ABI requires DF=0.
void native_campaign_ai_speech_queue_reclaim_tu15(
    SourcePhysicsContactMemory &,std::uint32_t queue);
// Original217970: admit into the actual policy queue, postpone later rows,
// propagate cancellation and reclaim. No sound provider result is supplied.
NativeCampaignAiSpeechQueueAdmission native_campaign_ai_speech_queue_admit_tu15(
    SourcePhysicsContactMemory &,std::uint32_t queue,std::uint32_t datum,
    NativeCampaignAudioFp &,unsigned available_extended_temporaries);
}
