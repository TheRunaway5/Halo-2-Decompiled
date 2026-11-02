// Native kernels: unit speech (TU1.5 sound manager). 18F510 itself belongs to
// the object-loop group; it calls these routines through the live world.
#include "audio/native_tu15_sound_kernel.hpp"
#include "halo2/native_campaign_speech_envelope_tu15_binding.hpp"
#include "halo2/native_campaign_speech_smoothing_tu15_binding.hpp"

namespace halo2::native_tu15_sound {
std::span<const Kernel> unit_speech_kernels(){
    static constexpr Kernel kernels[]{
        {0x18f7a0u,+[](M& memory,Abi& abi,FP fp,const O& observer,const Services&){
            native_campaign_speech_smoothing_tu15_run(memory,abi,fp,observer);
        },{0xf8u,0u,0xffu}},
        {0x21f860u,
        +[](M& memory,Abi& abi,FP fp,const O&,const Services&){
            native_campaign_speech_envelope_tu15_run(memory,abi,fp);
        },{0xf8u,0u,0xffu}}};
    return kernels;
}
} // namespace halo2::native_tu15_sound
