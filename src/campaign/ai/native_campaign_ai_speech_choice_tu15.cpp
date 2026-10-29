#include "halo2/native_campaign_ai_speech_choice_tu15.hpp"
#include "halo2/native_campaign_audio_ducking.hpp"
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value) {
    const std::array<std::byte,4> bytes{std::byte(value),std::byte(value>>8u),std::byte(value>>16u),std::byte(value>>24u)};
    if(!observe){memory.write(address,bytes);return;}
    SourcePhysicsContactRecordStore event{site,address,{}, {bytes.begin(),bytes.end()}};
    event.before.resize(4);memory.read(address,event.before);memory.write(address,bytes);observe(event);
}
struct RawCandidates {
    SourcePhysicsContactMemory &memory;std::uint32_t address;const SourcePhysicsContactRecordObserver &observe;
    std::uint32_t weight(std::int16_t index)const{return read(memory,address+std::uint32_t(index)*20u+0x10u);}
    void discard(std::int16_t index){publish(memory,observe,0x2173d7,address+std::uint32_t(index)*20u+0x10u,0);}
};
struct LocalCandidates {
    std::span<NativeCampaignAiSpeechCandidate> values;
    std::uint32_t weight(std::int16_t index)const{return values[std::uint16_t(index)].weight_bits;}
    void discard(std::int16_t index){values[std::uint16_t(index)].weight_bits=0;}
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace native_campaign_ducking_detail;
bool at_least(__m128 left,__m128 right) {
    unsigned char result;asm volatile("comiss %2,%1; setae %0":"=qm"(result):"x"(left),"x"(right):"cc","memory");return result!=0;
}
std::uint32_t scaled_target(std::uint32_t high,std::uint32_t normalization,std::uint32_t total) {
    const auto integer=std::int32_t(high);std::uint32_t result;
    // One real extended temporary: normalize the unsigned high16 draw, then
    // scale by the actual single-precision total before the final store.
    asm volatile("fildl %1; fmuls %2; fmuls %3; fstps %0":"=m"(result):"m"(integer),"m"(normalization),"m"(total):"memory","st");return result;
}
template<class Candidates> NativeCampaignAiSpeechChoice choose(SourcePhysicsContactMemory &memory,Candidates &candidates,
    std::int16_t count,NativeCampaignAudioFp &fp,unsigned available,const SourcePhysicsContactRecordObserver &observe) {
    if(count==1)return {0,false,false,0,0,0};
    Environment environment(fp);const auto zero=_mm_setzero_ps();auto maximum=zero,total=zero;
    for(std::int16_t index=0;index<count;++index) {
        const auto weight=scalar(candidates.weight(index));if(above(weight,maximum))maximum=weight;
    }
    if(count>0) {
        const auto threshold=mul(maximum,scalar(read(memory,0x456910u)));
        for(std::int16_t index=0;index<count;++index) {
            const auto weight=scalar(candidates.weight(index));
            if(at_least(weight,threshold))total=add(weight,total);else candidates.discard(index);
        }
    }
    const auto owner=read(memory,0x4f3cd8u),previous=read(memory,owner);
    const auto next=previous*1664525u+1013904223u;publish(memory,observe,0x2173fa,owner,next);
    if(available<1)throw std::runtime_error("Native speech choice requires one available extended temporary");
    const auto normalization=read(memory,0x46a160u);
    const auto target=scaled_target(next>>16u,normalization,bits(total));
    auto cumulative=zero;NativeCampaignAiSpeechChoice result{-1,true,false,0,target,0};
    for(std::int16_t index=0;index<count;++index) {
        const auto weight=scalar(candidates.weight(index));result.inspected_score=true;
        result.last_term_bits=bits(weight);
        if(above(weight,zero)) {
            cumulative=add(weight,cumulative);result.last_term_bits=bits(cumulative);
            if(at_least(cumulative,scalar(target))) {
                result.selected=index;result.cumulative_bits=bits(cumulative);return result;
            }
        }
    }
    result.cumulative_bits=bits(cumulative);return result;
}
#endif
}
NativeCampaignAiSpeechChoice native_campaign_ai_speech_choice_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t address,std::int16_t count,NativeCampaignAudioFp &fp,unsigned available,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    RawCandidates candidates{memory,address,observe};return choose(memory,candidates,count,fp,available,observe);
#else
    (void)memory;(void)address;(void)count;(void)fp;(void)available;(void)observe;throw std::runtime_error("Native speech choice FP backend unavailable");
#endif
}
NativeCampaignAiSpeechChoice native_campaign_ai_speech_choice_tu15(SourcePhysicsContactMemory &memory,
    std::span<NativeCampaignAiSpeechCandidate> values,std::int16_t count,NativeCampaignAudioFp &fp,unsigned available,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(count>1&&std::size_t(count)>values.size())throw std::invalid_argument("Native speech candidate owner is too small");
    LocalCandidates candidates{values};return choose(memory,candidates,count,fp,available,observe);
#else
    (void)memory;(void)values;(void)count;(void)fp;(void)available;(void)observe;throw std::runtime_error("Native speech choice FP backend unavailable");
#endif
}
}
