#pragma once
#include "halo2/native_campaign_audio_ducking.hpp"

namespace halo2 {
namespace native_campaign_fades_detail {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace native_campaign_ducking_detail;
inline constexpr std::uint32_t silence=0xc2800000u,one=0x3f800000u;
// These are numerical primitives on the physical FPU. Long double transports
// an actual extended value, never a virtual x87 stack or instruction machine.
inline bool narrow_interval(std::uint32_t first,std::uint32_t second) {
    const std::uint32_t epsilon=0x38d1b717u;unsigned char narrow;
    asm volatile("flds %1; fsubs %2; fabs; flds %3; fcomip %%st(1),%%st; fstp %%st(0); seta %0"
        :"=qm"(narrow):"m"(first),"m"(second),"m"(epsilon):"cc","memory","st","st(1)");
    return narrow!=0;
}
inline std::uint32_t fraction(std::uint32_t first,std::uint32_t second,std::uint32_t current,std::uint8_t policy) {
    if(narrow_interval(first,second))return unsigned(above(scalar(first),scalar(current)))==policy?0u:one;
    const auto distance=sub(scalar(current),scalar(first));
    const auto extent=sub(scalar(second),scalar(first));
    auto result=div(distance,extent);
    if(above(_mm_setzero_ps(),result))result=_mm_setzero_ps();
    else if(above(result,scalar(one)))result=scalar(one);
    return bits(result);
}
inline long double decibels_to_amplitude(std::uint32_t input) {
    auto decibels=scalar(input);
    if(above(scalar(silence),decibels))decibels=scalar(silence);
    else if(above(decibels,_mm_setzero_ps()))decibels=_mm_setzero_ps();
    const auto bounded=bits(decibels);
    const std::uint32_t twentieth=0x3d4ccccdu,ln10=0x40135d8eu;
    long double result;
    // 10^(dB/20): split the binary exponent into its integral and fractional
    // parts under the caller's precision and rounding controls.
    asm volatile("flds %1; fmuls %2; fmuls %3; fldl2e; fmulp; fld %%st(0); frndint; fxch %%st(1); "
        "fsub %%st(1),%%st; f2xm1; fld1; faddp; fscale; fstp %%st(1); fstpt %0"
        :"=m"(result):"m"(bounded),"m"(twentieth),"m"(ln10):"memory","st","st(1)","st(2)");
    return result;
}
inline std::uint32_t rounded_amplitude(long double amplitude) {
    std::uint32_t result;
    asm volatile("fldt %1; fstps %0":"=m"(result):"m"(amplitude):"memory","st");return result;
}
inline std::uint32_t linear_amplitude(std::uint32_t from,long double to,std::uint32_t position) {
    std::uint32_t result;
    asm volatile("fldt %1; fsubs %2; fmuls %3; fadds %2; fstps %0"
        :"=m"(result):"m"(to),"m"(from),"m"(position):"memory","st");return result;
}
inline std::uint32_t curved_position(std::uint32_t from,std::uint32_t to,std::uint32_t position) {
    unsigned char increasing;
    asm volatile("flds %1; flds %2; fcomip %%st(1),%%st; fstp %%st(0); seta %0"
        :"=qm"(increasing):"m"(from),"m"(to):"cc","memory","st","st(1)");
    std::uint32_t result;
    if(increasing)asm volatile("flds %1; fsqrt; fstps %0":"=m"(result):"m"(position):"memory","st");
    else asm volatile("flds %2; fsubs %1; fsqrt; fsubrs %2; fstps %0"
        :"=m"(result):"m"(position),"m"(one):"memory","st");
    return result;
}
inline std::uint32_t amplitude_to_decibels(std::uint32_t input) {
    const auto amplitude=scalar(input);auto bounded=amplitude;
    if(above(_mm_setzero_ps(),amplitude))bounded=_mm_setzero_ps();
    else if(above(amplitude,scalar(one)))bounded=scalar(one);
    std::uint32_t result=silence;
    if(above(amplitude,_mm_setzero_ps())) {
        const auto value=bits(bounded);const std::uint32_t twenty=0x41a00000u;unsigned char below;std::uint32_t rounded;
        asm volatile("flds %2; fldlg2; fxch %%st(1); fyl2x; fmuls %3; fsts %0; flds %4; "
            "fcomip %%st(1),%%st; fstp %%st(0); seta %1"
            :"=m"(rounded),"=qm"(below):"m"(value),"m"(twenty),"m"(silence):"cc","memory","st","st(1)");
        if(!below)result=above(scalar(rounded),_mm_setzero_ps())?0u:rounded;
    }
    if(above(scalar(silence),scalar(result)))return silence;
    return above(scalar(result),_mm_setzero_ps())?0u:result;
}
template<class State> std::uint32_t sample(State& state,std::uint32_t datum) {
    auto table=state.sounds();auto sound=table.row(datum);
    if(!(sound.flags()&0x80u))return 0u;
    const auto old_start=sound.start();auto clock=state.clock();
    if(old_start==0xffffffffu || sound.end()==0xffffffffu) {
        const auto now=clock.milliseconds();sound.start(old_start+now+1u);
        const auto later=clock.milliseconds();const auto old_end=sound.end();sound.end(old_end+later+1u);
    }
    const auto start=sound.start();const auto now=clock.milliseconds();const auto end=sound.end();
    const bool increasing=std::bit_cast<std::int32_t>(start)<std::bit_cast<std::int32_t>(end);
    const auto origin=increasing?end:start;
    const auto current=bits(signed_milliseconds(now-origin));
    const auto second=bits(signed_milliseconds(end-origin));
    const auto first=bits(signed_milliseconds(start-origin));
    const auto position=fraction(first,second,current,increasing);
    const auto from=increasing?sound.gain():silence;
    const auto to=increasing?0u:sound.gain();
    const auto curve=sound.curve();std::uint32_t result{};
    if(curve==0 || curve==1) {
        const auto from_amplitude=rounded_amplitude(decibels_to_amplitude(from));
        const auto to_extended=decibels_to_amplitude(to);
        std::uint32_t mixed;
        if(curve==0)mixed=linear_amplitude(from_amplitude,to_extended,position);
        else {
            const auto to_amplitude=rounded_amplitude(to_extended);
            const auto shaped=curved_position(from_amplitude,to_amplitude,position);
            mixed=bits(add(mul(sub(scalar(to_amplitude),scalar(from_amplitude)),scalar(shaped)),scalar(from_amplitude)));
        }
        result=amplitude_to_decibels(mixed);
    }
    if(above(scalar(silence),scalar(result)))return silence;
    return above(scalar(result),_mm_setzero_ps())?0u:result;
}
#endif
}
// The State service provides typed, lazy sound-table/row and clock views. Each
// view retains its actual owner; subsequent field reads observe earlier writes,
// including requests whose source and destination are the same sound datum.
template<class State> std::uint32_t native_campaign_sample_fade(State& state,std::uint32_t datum,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    native_campaign_fades_detail::Environment environment(fp);return native_campaign_fades_detail::sample(state,datum);
#else
    (void)state;(void)datum;(void)fp;throw std::runtime_error("Exact native campaign fade FP backend unavailable");
#endif
}
inline std::uint32_t native_campaign_fade_fraction(std::uint32_t first,std::uint32_t second,std::uint32_t current,
    std::uint8_t policy,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    native_campaign_fades_detail::Environment environment(fp);return native_campaign_fades_detail::fraction(first,second,current,policy);
#else
    (void)first;(void)second;(void)current;(void)policy;(void)fp;throw std::runtime_error("Exact native campaign fade FP backend unavailable");
#endif
}
inline std::uint32_t native_campaign_fade_decibels(std::uint32_t amplitude,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    native_campaign_fades_detail::Environment environment(fp);return native_campaign_fades_detail::amplitude_to_decibels(amplitude);
#else
    (void)amplitude;(void)fp;throw std::runtime_error("Exact native campaign fade FP backend unavailable");
#endif
}
template<class State> void native_campaign_request_fade(State& state,std::uint32_t from,std::uint32_t to,
    std::int16_t curve,std::uint32_t seconds,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    using namespace native_campaign_fades_detail;Environment environment(fp);auto table=state.sounds();
    if(from!=0xffffffffu) {
        auto sound=table.row(from);const bool active=(sound.flags()&0x80u)!=0;
        sound.gain(active?sample(state,from):silence,false,active);
        const auto milliseconds=mul(scalar(seconds),scalar(0x447a0000u));
        const auto duration=std::uint32_t(_mm_cvttss_si32(milliseconds));
        sound.curve(curve,false);sound.start(0xffffffffu,false);sound.end(duration,false);sound.activate(false);
    }
    if(to!=0xffffffffu) {
        auto sound=table.row(to);const auto gain=sample(state,to);
        const auto milliseconds=mul(scalar(seconds),scalar(0x447a0000u));
        sound.gain(gain,true,true);
        const auto duration=std::uint32_t(_mm_cvttss_si32(milliseconds));
        sound.curve(curve,true);sound.start(duration,true);sound.end(0xffffffffu,true);sound.activate(true);
    }
#else
    (void)state;(void)from;(void)to;(void)curve;(void)seconds;(void)fp;throw std::runtime_error("Exact native campaign fade FP backend unavailable");
#endif
}
} // namespace halo2
