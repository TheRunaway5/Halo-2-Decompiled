#include "halo2/native_campaign_speech_smoothing_tu15_binding.hpp"
#include "halo2/native_campaign_speech_smoothing_tu15.hpp"

namespace halo2 {
// At the admitted18F510 call, EAX/ECX/EDX are reloaded before use and
//18F5E3 ADD retires arithmetic EFLAGS. All SIMD and non-arithmetic flag
// obligations remain preserved; no generic indirect-caller claim is made.
namespace {
void publish(SourcePhysicsContactRuntimeAbi &abi,const NativeCampaignSpeechSmoothingProgressTu15 &progress) {
    // Scalar memory loads clear the upper lanes. Difference arithmetic
    // preserves the sample's upper lanes unless a bound was selected.
    for(const auto &quantity:{progress.lower_bound,progress.upper_bound,
            progress.current_level,progress.updated_level})
        if(quantity)abi.xmm[1]={*quantity,0,0,0};
    if(progress.difference)abi.xmm[0][0]=*progress.difference;
    if(progress.selected_delta) {
        if(progress.selected_delta->selected_bound)abi.xmm[0].fill(0);
        abi.xmm[0][0]=progress.selected_delta->bits;
    }
}
}
void native_campaign_speech_smoothing_tu15_run(SourcePhysicsContactMemory &memory,SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fallback,const SourcePhysicsContactRecordObserver &observer) {
    const auto policy=abi.floating_point.value_or(fallback);
    NativeCampaignAudioFp fp{policy.x87_control_word,abi.x87_status,policy.mxcsr};
    struct Publication {
        SourcePhysicsContactRuntimeAbi &abi;NativeCampaignAudioFp &fp;
        ~Publication() {
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
            abi.x87_status=fp.status;
        }
    } publication{abi,fp};
    const auto sp=abi.gp[4];
    NativeCampaignSpeechSmoothingProgressTu15 progress;
    try {progress=native_campaign_speech_smoothing_tu15(memory,abi.gp[0],abi.xmm[0][0],fp,observer);}
    catch(const NativeCampaignSpeechSmoothingPendingErrorTu15 &error) {
        fp=error.floating_point();publish(abi,error.progress());
        std::rethrow_exception(error.cause());
    }
    publish(abi,progress);
    std::array<std::byte,4> return_word{};memory.read(sp,return_word);
    abi.gp[4]=sp+4u;
}
}
