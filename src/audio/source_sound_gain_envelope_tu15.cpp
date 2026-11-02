#include "halo2/source_sound_gain_envelope_tu15.hpp"
#include "halo2/native_campaign_audio_listeners.hpp"
#include "halo2/sound_spatial.hpp"
namespace halo2 {namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace native_campaign_listeners_detail;
long double amplitude(std::uint32_t gain){
    auto level=scalar(gain);const auto floor=scalar(0xc2800000u),zero=_mm_setzero_ps();
    if(above(floor,level))gain=0xc2800000u;else if(above(level,zero))gain=0;
    const std::uint32_t exponent_scale=0x3d4ccccdu,ln10=0x40135d8eu;long double result;
    asm volatile("flds %1; fmuls %2; fmuls %3; fldl2e; fmulp; fld %%st(0); frndint; "
        "fxch %%st(1); fsub %%st(1),%%st; f2xm1; fld1; faddp; fscale; fstp %%st(1); fstpt %0"
        :"=m"(result):"m"(gain),"m"(exponent_scale),"m"(ln10):"memory","st","st(1)","st(2)");return result;
}
std::uint32_t envelope(const SourceSoundGainEnvelopeTu15& metadata,std::uint32_t control,
    const SoundDriverFloatingPointContext& controls){
    const auto upper=bits(add(scalar(metadata.scale_upper),scalar(metadata.class_upper)));
    const auto lower=bits(add(scalar(metadata.scale_lower),scalar(metadata.class_lower)));
    const auto lower_extended=amplitude(lower);std::uint32_t quiet;
    asm volatile("fldt %1; fstps %0":"=m"(quiet):"m"(lower_extended):"memory","st");
    const auto loud=amplitude(upper);std::uint32_t selected;
    asm volatile("fldt %1; fsubs %2; fmuls %3; fadds %2; fstps %0"
        :"=m"(selected):"m"(loud),"m"(quiet),"m"(control):"memory","st");
    return source_sound_spatial_gain_db(selected,{0x3f800000u,0xc2800000u,0x41a00000u},controls);
}
#endif
}
std::uint32_t source_sound_gain_envelope_tu15(const SourceSoundGainEnvelopeTu15& metadata,
    std::uint32_t control,const SoundDriverFloatingPointContext& controls){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeCampaignAudioFp fp{controls.x87_control_word,0,controls.mxcsr};
    native_campaign_ducking_detail::Environment environment(fp);return envelope(metadata,control,controls);
#else
    (void)metadata;(void)control;(void)controls;throw std::runtime_error("Native gain-envelope FP backend unavailable");
#endif
}
std::array<std::uint32_t,4> source_loop_combined_gain_tu15(const std::array<std::uint32_t,4>& amplitudes,
    const SourceSoundGainEnvelopeTu15& metadata,std::uint32_t control,std::uint32_t parent,
    std::uint32_t ducked,const SoundDriverFloatingPointContext& controls){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeCampaignAudioFp fp{controls.x87_control_word,0,controls.mxcsr};
    native_campaign_ducking_detail::Environment environment(fp);std::array<std::uint32_t,4> result;
    for(unsigned i=0;i<4;++i){
        const auto db=source_sound_spatial_gain_db(amplitudes[i],{0x3f800000u,0xc2800000u,0x41a00000u},controls);
        const auto selected=envelope(metadata,control,controls);
        result[i]=bits(add(add(scalar(selected),add(scalar(ducked),scalar(db))),scalar(parent)));
    }
    return result;
#else
    (void)amplitudes;(void)metadata;(void)control;(void)parent;(void)ducked;(void)controls;
    throw std::runtime_error("Native combined gain-envelope FP backend unavailable");
#endif
}
}
