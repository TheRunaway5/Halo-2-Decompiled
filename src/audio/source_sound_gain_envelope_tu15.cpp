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

SourceCombinedDriverPacketTu15 source_combined_driver_packet_tu15(
    std::uint32_t driver_flags, std::uint32_t voice_gain,
    const std::array<std::uint32_t,4>& channel_gains,
    const SoundDriverFloatingPointContext& controls) {
    if (!(driver_flags & 0x20u))
        throw std::invalid_argument("Combined driver packet requires the original four-gain flag");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeCampaignAudioFp fp{controls.x87_control_word,0,controls.mxcsr};
    native_campaign_ducking_detail::Environment environment(fp);
    long double power = 0.L;
    // 2267A0 squares each clamped amplitude before accumulating it. 2267FB
    // spills the square root to single precision before the dB conversion.
    for (const auto gain : channel_gains) {
        const auto linear = amplitude(gain);
        asm volatile("fldt %1; fmul %%st(0),%%st(0); fldt %0; faddp; fstpt %0"
                     : "+m"(power) : "m"(linear) : "st","st(1)");
    }
    std::uint32_t rounded;
    asm volatile("fldt %1; fsqrt; fstps %0" : "=m"(rounded) : "m"(power) : "st");
    const auto common = source_sound_spatial_gain_db(rounded,{0x3f800000u,0xc2800000u,0x41a00000u},controls);
    const auto flags = 0x20u | (driver_flags & 0x58u) | ((driver_flags & 2u) << 1);
    return {flags,common,voice_gain,channel_gains};
#else
    (void)driver_flags;(void)voice_gain;(void)channel_gains;(void)controls;
    throw std::runtime_error("Native combined driver gain FP backend unavailable");
#endif
}

SourceCombinedMixBinsTu15 source_combined_driver_routes_tu15(
    const SourceCombinedDriverPacketTu15& packet, const SourceCombinedDriverRoutingTu15& routing,
    const SoundDriverFloatingPointContext& controls) {
    if (routing.auxiliary_bin >= 32 && routing.auxiliary_bin != 0xffffu)
        throw std::invalid_argument("Combined driver auxiliary bin exceeds its source range");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeCampaignAudioFp fp{controls.x87_control_word,0,controls.mxcsr};
    native_campaign_ducking_detail::Environment environment(fp);
    // 2B9450 normalizes from [-64,0], rounds the SSE multiply and subtract
    // independently, and only then truncates to driver attenuation units.
    const auto convert = [](__m128 gain) {
        const auto zero = _mm_setzero_ps();
        const auto one = _mm_set_ss(1.f);
        const auto floor = _mm_set_ss(-64.f);
        const auto scale = _mm_set_ss(6400.f);
        auto normalized = _mm_div_ss(sub(gain,floor),sub(zero,floor));
        if (above(zero,normalized)) normalized = zero;
        else if (above(normalized,one)) normalized = one;
        return _mm_cvttss_si32(sub(mul(normalized,scale),scale));
    };
    const auto bins = (packet.flags & 8u) ? std::array{0u,1u,4u,5u} : std::array{6u,7u,8u,9u};
    SourceCombinedMixBinsTu15 result;
    const auto append = [&](unsigned bin, __m128 gain) {
        if (result.count == result.routes.size())
            throw std::invalid_argument("Combined driver routes exceed the original eight slots");
        result.routes[result.count++] = {bin,convert(gain)};
    };
    const auto send_base = add(scalar(packet.voice_gain),scalar(packet.common_gain));
    for (unsigned i=0;i<bins.size();++i) append(bins[i],scalar(packet.channel_gains[i]));
    if (routing.category == 0) append(10,send_base);
    if (routing.category == 1) {
        const auto send = add(scalar(routing.plugin_send_gain),send_base);
        append(10,send);
        append(10,send);
    }
    const auto available = result.routes.size() - result.count;
    const auto zero = _mm_setzero_ps();
    // 2B96B0 reads the layout independently from the dry-bin bypass flag.
    // It only adds a complete group when the original list has room.
    const auto pair = [&](unsigned left, unsigned right) {
        if (available >= 2) {
            append(left,zero);
            append(right,zero);
        }
    };
    switch (routing.mode) {
    case 1:
        pair(bins[0],bins[1]);
        break;
    case 2:
        if (routing.layout) pair(bins[2],bins[3]);
        else pair(bins[0],bins[1]);
        break;
    case 3:
        if (routing.layout) pair(2,3);
        else if (available >= 4) {
            const auto half_amplitude = sub(zero,scalar(0x40c0a8c1u));
            const auto silence = scalar(0xc2800000u);
            append(bins[0],half_amplitude);
            append(bins[0],silence);
            append(bins[1],half_amplitude);
            append(bins[1],silence);
        }
        break;
    default:
        break;
    }
    if (routing.auxiliary_bin != 0xffffu) {
        if (routing.category == 0) append(routing.auxiliary_bin,zero);
        if (routing.category == 1) {
            const auto send = add(scalar(routing.plugin_send_gain),zero);
            append(routing.auxiliary_bin,send);
            append(routing.auxiliary_bin,send);
        }
    }
    return result;
#else
    (void)packet;(void)routing;(void)controls;
    throw std::runtime_error("Native combined driver routing FP backend unavailable");
#endif
}
}
