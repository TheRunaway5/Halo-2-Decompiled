#pragma once
#include "halo2/native_campaign_audio_leaves.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

namespace halo2 {
enum class NativeCampaignDuckingGain { decrease,clamp,release,ordinary };
class NativeCampaignDuckingState {
public:
    virtual ~NativeCampaignDuckingState()=default;
    virtual std::uint32_t current_milliseconds()const=0;
    virtual std::uint32_t last_milliseconds()const=0;
    virtual std::uint32_t elapsed_milliseconds()const=0;
    virtual std::uint32_t accumulated_seconds()const=0;
    virtual std::uint32_t effect_seconds()const=0;
    virtual std::uint32_t frame_seconds()const=0;
    // Resolves and retains this frame's actual game owner before subsequent
    // audio publications. Later field reads stay lazy on that same owner.
    virtual bool game_present()const=0;
    virtual bool game_active()const=0;
    virtual bool campaign()const=0;
    virtual bool paused()const=0;
    virtual std::uint32_t gain()const=0;
    virtual std::uint32_t delay_milliseconds()const=0;
    virtual std::uint32_t delay_seconds()const=0;
    virtual std::uint32_t fade_out_seconds()const=0;
    virtual std::uint32_t fade_in_seconds()const=0;
    virtual std::uint8_t activity_gate(unsigned)const=0;
    virtual std::int32_t activity_counter(unsigned)const=0;
    virtual void last_milliseconds(std::uint32_t)=0;
    virtual void elapsed_milliseconds(std::uint32_t)=0;
    virtual void accumulated_seconds(std::uint32_t)=0;
    virtual void effect_seconds(std::uint32_t)=0;
    virtual void frame_seconds(std::uint32_t)=0;
    virtual void delay_milliseconds(std::uint32_t,bool initial)=0;
    virtual void gain(std::uint32_t,NativeCampaignDuckingGain)=0;
};
// Complete 12E9A0 query. The primary activity byte does not short-circuit the
// remaining reads; seven other gates and two signed counters keep source order.
inline bool native_campaign_audio_activity(const NativeCampaignDuckingState& state) {
    const bool primary=state.activity_gate(0)!=0;
    for(unsigned gate=1;gate<8;++gate)if(state.activity_gate(gate))return true;
    if(state.activity_counter(0)>0)return true;
    if(state.activity_counter(1)>0)return true;
    return primary;
}
namespace native_campaign_ducking_detail {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    unsigned host_mxcsr_{};
    NativeCampaignAudioFp& fp_;
public:
    explicit Environment(NativeCampaignAudioFp& fp):fp_(fp) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native campaign ducking requires masked original FP exceptions");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());const std::uint16_t empty=0xffff;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment() {
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);
    }
};
inline __m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
inline std::uint32_t bits(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
inline bool above(__m128 left,__m128 right) {
    unsigned char value;asm volatile("comiss %2,%1; seta %0":"=qm"(value):"x"(left),"x"(right):"cc","memory");return value!=0;
}
// One real SSE operation per arithmetic dependency, retaining NaN operand order.
inline __m128 add(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
inline __m128 sub(__m128 left,__m128 right){asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");return left;}
inline __m128 mul(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
inline __m128 div(__m128 left,__m128 right){asm volatile("divss %1,%0":"+x"(left):"x"(right):"memory");return left;}
inline __m128 signed_milliseconds(std::uint32_t value) {
    return _mm_cvtsi32_ss(_mm_setzero_ps(),std::bit_cast<std::int32_t>(value));
}
#endif
}
// Complete 129500 and its closed 33B590/12E9A0 dependency semantics. The clock
// provider reads the actual imported millisecond cell; no timing result is
// synthesized, nor is an unknown original owner resumed. All retained writes
// and reads belong to typed phases, with no virtual CPU/register simulation.
inline void native_campaign_update_ducking(NativeCampaignDuckingState& state,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    using namespace native_campaign_ducking_detail;
    Environment environment(fp);
    const auto now=state.current_milliseconds();
    const auto previous=state.last_milliseconds();
    const auto old_seconds=scalar(state.accumulated_seconds());
    const auto elapsed=now-previous;
    state.last_milliseconds(now);
    const auto ticks=state.elapsed_milliseconds();
    const auto integer_seconds=signed_milliseconds(elapsed);
    const auto seconds=mul(integer_seconds,scalar(0x3a83126fu));
    const auto accumulated=add(old_seconds,seconds);
    state.elapsed_milliseconds(ticks+elapsed);
    const bool present=state.game_present();
    state.accumulated_seconds(bits(accumulated));
    const auto old_effect_seconds=scalar(state.effect_seconds());
    const auto frame_seconds=mul(integer_seconds,scalar(0x3a83126fu));
    const auto effect_seconds=add(old_effect_seconds,seconds);
    state.frame_seconds(bits(frame_seconds));
    state.effect_seconds(bits(effect_seconds));
    if(!present || !state.game_active() || !state.campaign()) {
        state.gain(0x3f800000u,NativeCampaignDuckingGain::ordinary);return;
    }
    if(state.paused()) {
        if(state.delay_milliseconds()==0xffffffffu) {
            const auto delay=mul(scalar(state.delay_seconds()),scalar(0x447a0000u));
            state.delay_milliseconds(std::uint32_t(_mm_cvttss_si32(delay)),true);
        }
        const auto previous_delay=state.delay_milliseconds();
        if(std::bit_cast<std::int32_t>(previous_delay)>0) {
            const auto remaining=previous_delay-elapsed;
            state.delay_milliseconds(std::bit_cast<std::int32_t>(remaining)>0?remaining:0u,false);
        }
        if(state.delay_milliseconds()!=0)return;
        auto duration=scalar(0x3a83126fu);
        if(!above(duration,scalar(state.fade_out_seconds())))duration=scalar(state.fade_out_seconds());
        const auto step=div(frame_seconds,duration);
        const auto distance=sub(_mm_setzero_ps(),scalar(state.gain()));
        const auto decreasing=sub(_mm_setzero_ps(),step);
        if(above(decreasing,distance)) {
            state.gain(bits(add(decreasing,scalar(state.gain()))),NativeCampaignDuckingGain::decrease);
        } else {
            const auto change=above(distance,step)?step:distance;
            state.gain(bits(add(change,scalar(state.gain()))),NativeCampaignDuckingGain::clamp);
        }
        return;
    }
    if(native_campaign_audio_activity(state))return;
    auto duration=scalar(0x3a83126fu);
    if(!above(duration,scalar(state.fade_in_seconds())))duration=scalar(state.fade_in_seconds());
    const auto seconds_now=scalar(state.frame_seconds());
    const auto distance=sub(scalar(0x3f800000u),scalar(state.gain()));
    const auto step=div(seconds_now,duration);
    const auto decreasing=sub(_mm_setzero_ps(),step);
    const auto change=above(decreasing,distance)?decreasing:(above(distance,step)?step:distance);
    state.gain(bits(add(scalar(state.gain()),change)),NativeCampaignDuckingGain::release);
    state.delay_milliseconds(0xffffffffu,false);
#else
    (void)state;(void)fp;throw std::runtime_error("Exact native campaign ducking FP backend unavailable");
#endif
}
} // namespace halo2
