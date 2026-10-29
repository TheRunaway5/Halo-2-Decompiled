#include "halo2/native_campaign_ai_speech_followup_tu15.hpp"
#include "halo2/native_campaign_audio_ducking.hpp"
#include <bit>
#include <xmmintrin.h>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(i*8u);return value;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t value,unsigned size=4) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(i*8u));m.write(p,std::span(bytes).first(size));
}
std::int32_t word(const SourcePhysicsContactMemory &m,std::uint32_t p) {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(m,p,2)));
}
class SimdEnvironment {
    NativeCampaignAudioFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeCampaignAudioFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native speech channel requires masked SIMD traps");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
using namespace native_campaign_ducking_detail;
__m128 integer(std::int32_t value) {
    __m128 result;asm volatile("cvtsi2ssl %1,%0":"=x"(result):"rm"(value):"memory");return result;
}
std::uint32_t truncate(__m128 value) {
    std::uint32_t result;asm volatile("cvttss2sil %1,%0":"=r"(result):"x"(value):"memory");return result;
}
std::uint32_t byte_channel(__m128 value) {const auto result=truncate(value);return result<=255u?result:255u;}
}
NativeCampaignAiSpeechUnitChannelEffect native_campaign_ai_speech_unit_channel_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t unit,std::int16_t channel,std::uint32_t target,std::uint32_t duration,NativeCampaignAudioFp &fp) {
    const auto objects=read(m,0x4ecbd0u),headers=read(m,objects+0x44u),body=read(m,headers+(unit&0xffffu)*12u+8u);
    const auto state=body+std::uint32_t(word(m,body+0x33eu));
    if((read(m,state+0x5cu)>>16u)==0xffffu)return {};
    const auto zero=_mm_setzero_ps();
    if(channel!=-1&&channel<13) {write(m,state+0x6cu,std::uint16_t(channel),2);write(m,state+0x68u,target);}
    else {write(m,state+0x6cu,0xffffu,2);write(m,state+0x68u,0);}
    SimdEnvironment environment(fp);
    if(above(scalar(duration),zero)) {
        const auto clock=read(m,0x51df54u);const auto rate=integer(word(m,clock+2u));
        const auto scaled=mul(rate,scalar(duration));const auto one=scalar(read(m,0x46a0d0u));
        const auto inverse=div(one,scaled);write(m,state+0x60u,bits(inverse));write(m,state+0x64u,0);
        return {NativeCampaignAiSpeechUnitChannelPhase::blend,bits(inverse),bits(scaled)};
    }
    for(unsigned n=0;n<13;++n)write(m,state+0x6eu+n,byte_channel(zero),1);
    const auto current=scalar(read(m,state+0x68u));const auto scaled=mul(current,scalar(read(m,0x46a270u)));
    write(m,state+0x6eu+std::uint32_t(std::int32_t(channel)),byte_channel(scaled),1);
    write(m,state+0x60u,0);write(m,state+0x64u,0);
    return {NativeCampaignAiSpeechUnitChannelPhase::immediate,bits(scaled),0};
}
NativeCampaignAiSpeechActorChannelEffect native_campaign_ai_speech_actor_channel_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t actor,std::int16_t channel,std::uint16_t timer,NativeCampaignAudioFp &fp) {
    const auto offset=(actor&0xffffu)*0x888u,actors=read(m,0x5020c8u),record=read(m,actors+0x44u)+offset;
    if(channel<0||channel>=13)return {};
    const auto unit=read(m,record+0x18u),target=read(m,0x46a0d0u);
    const auto effect=native_campaign_ai_speech_unit_channel_tu15(m,unit,channel,target,0x3f000000u,fp);
    write(m,record+0x6ceu,timer,2);return {true,target,effect};
}
NativeCampaignAiSpeechTickConversion native_campaign_ai_speech_seconds_to_ticks_tu15(const SourcePhysicsContactMemory &m,
    std::uint32_t seconds,NativeCampaignAudioFp &fp,unsigned available) {
    const auto clock=read(m,0x51df54u);const auto rate=word(m,clock+2u);
    Environment environment(fp);const auto scaled=bits(mul(integer(rate),scalar(seconds)));
    if(available<1)throw std::runtime_error("Native speech tick conversion requires one available extended temporary");
    std::uint32_t ticks;asm volatile("flds %1; fistpl %0":"=m"(ticks):"m"(scaled):"memory","st");return {ticks,scaled};
}
}
