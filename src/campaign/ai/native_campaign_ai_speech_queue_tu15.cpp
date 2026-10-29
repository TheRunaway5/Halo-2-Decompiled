#include "halo2/native_campaign_ai_speech_queue_tu15.hpp"
#include "halo2/native_campaign_audio_ducking.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <bit>
#include <optional>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(i*8u);return value;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t value,unsigned size=4) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(i*8u));m.write(p,std::span(bytes).first(size));
}
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::int32_t word(const SourcePhysicsContactMemory &m,std::uint32_t p) {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(m,p,2)));
}
std::uint32_t row(const SourcePhysicsContactMemory &m,std::uint32_t datum) {
    const auto pool=read(m,0x505e70u);return read(m,pool+0x44u)+(datum&0xffffu)*0x54u;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Timing {
    NativeCampaignAudioFp &fp_;unsigned available_;
    std::optional<native_campaign_ducking_detail::Environment> environment_;
public:
    NativeCampaignAiSpeechQueueMath outcome;
    Timing(NativeCampaignAudioFp &fp,unsigned available):fp_(fp),available_(available){}
    std::uint32_t ticks(std::int32_t rate,std::uint32_t factor) {
        if(!environment_)environment_.emplace(fp_);
        __m128 value;asm volatile("cvtsi2ssl %1,%0":"=x"(value):"rm"(rate):"memory");
        value=native_campaign_ducking_detail::mul(value,native_campaign_ducking_detail::scalar(factor));
        outcome.delay_computed=true;outcome.delay_bits=native_campaign_ducking_detail::bits(value);
        if(available_<1)throw std::runtime_error("Native speech queue requires one available extended temporary");
        std::uint32_t result;const auto bits=outcome.delay_bits;
        asm volatile("flds %1; fistpl %0":"=m"(result):"m"(bits):"memory","st");return result;
    }
};
void postpone(SourcePhysicsContactMemory &m,std::uint32_t candidate,std::uint32_t blocker,Timing &timing) {
    auto definition=0u;const auto globals=read(m,0x4ecc1cu);
    if(globals&&signed32(read(m,globals+0xc8u))>0) {
        const auto settings=read(m,globals+0xccu),tag=read(m,settings+0x64u);
        if(tag!=none){const auto table=read(m,0x4f0414u);definition=read(m,table+(tag&0xffffu)*16u+8u);}
    }
    const auto start=read(m,candidate+0x14u),blocked_start=read(m,blocker+0x14u);
    if(signed32(start)<signed32(blocked_start))return;
    const auto end=blocked_start+std::uint32_t(word(m,blocker+0x1cu));
    if(signed32(start)>=signed32(end))return;
    const auto entries=read(m,definition+4u),distance=start-blocked_start;
    const auto sign=std::uint32_t(signed32(distance)>>31),absolute=(distance^sign)-sign;
    const bool close=signed32(absolute)<3;
    const auto action=word(m,candidate+2u);
    const auto action_definition=entries+std::uint32_t(action)*0x60u;
    const auto active=read(m,blocker+0x40u,1);
    bool interrupt=false;
    if(active) {
        const auto blocking_unit=read(m,blocker+4u);
        if(blocking_unit==read(m,candidate+4u)&&word(m,candidate+0x22u)<11)interrupt=true;
    }
    if(!interrupt) {
        const auto priority=word(m,blocker+0x22u);
        if(word(m,action_definition+0x1au)>=priority)return;
        if(!active) {
            if(word(m,candidate+0x22u)>priority&&close)return;
            if(word(m,candidate+0x22u)==priority&&close&&read(m,candidate+0x44u)!=none&&read(m,blocker+0x44u)==none)return;
        }
    }
    const auto clock=read(m,0x51df54u);const auto rate=word(m,clock+2u);
    const auto delay=timing.ticks(rate,read(m,0x46a16cu));
    const auto duration=word(m,blocker+0x1cu);const auto current=read(m,blocker+0x14u);
    write(m,candidate+0x14u,current+std::uint32_t(duration)+delay);
}
#endif
}
void native_campaign_ai_speech_queue_cancel_tu15(SourcePhysicsContactMemory &m,std::uint32_t datum) {
    const auto offset=(datum&0xffffu)*0x54u,pool=read(m,0x505e70u);
    const auto first=read(m,pool+0x44u)+offset;auto next=read(m,first+0x50u);
    write(m,first+0x48u,1,1);
    while(next!=none) {
        const auto rows=read(m,pool+0x44u),current=rows+(next&0xffffu)*0x54u;
        const auto dependency=read(m,current+0x44u);
        if(dependency!=none) {
            const auto index=dependency&0xffffu;
            if(signed32(index)<signed32(read(m,pool+0x38u))) {
                const auto stride=read(m,pool+0x24u),related=rows+stride*index;
                const auto salt=word(m,related);
                if(salt!=0&&salt==signed32(dependency)>>16&&read(m,related+0x48u,1))write(m,current+0x48u,1,1);
            }
        }
        next=read(m,current+0x50u);
    }
}
void native_campaign_ai_speech_queue_reclaim_tu15(SourcePhysicsContactMemory &m,std::uint32_t queue) {
    auto link=queue+0x7d0u;if(read(m,link)==none)return;
    const auto pool=read(m,0x505e70u);
    do {
        const auto datum=read(m,link),rows=read(m,pool+0x44u),current=rows+(datum&0xffffu)*0x54u;
        if(read(m,current+0x48u,1)) {
            const auto next=read(m,current+0x50u),unit=read(m,rows+(datum&0xffffu)*0x54u+4u);
            if(unit!=none) {
                const auto objects=read(m,0x4ecbd0u),headers=read(m,objects+0x44u);
                const auto object=read(m,headers+(unit&0xffffu)*12u+8u);
                const auto speech=object+std::uint32_t(word(m,object+0x342u));
                if(read(m,speech+0x1cu)==datum)write(m,speech+0x1cu,none);
            }
            native_tu15_data_array_delete(m,pool,datum);
            write(m,link,next);write(m,queue+0x7d4u,read(m,queue+0x7d4u,2)-1u,2);
        } else link=current+0x50u;
    }while(read(m,link)!=none);
}
NativeCampaignAiSpeechQueueMath native_campaign_ai_speech_queue_postpone_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t candidate,std::uint32_t blocker,NativeCampaignAudioFp &fp,unsigned available) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Timing timing(fp,available);postpone(m,candidate,blocker,timing);return timing.outcome;
#else
    (void)m;(void)candidate;(void)blocker;(void)fp;(void)available;throw std::runtime_error("Native speech queue FP backend unavailable");
#endif
}
NativeCampaignAiSpeechQueueAdmission native_campaign_ai_speech_queue_admit_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t queue,std::uint32_t datum,NativeCampaignAudioFp &fp,unsigned available) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Timing timing(fp,available);bool admitted=false;
    const auto pool=read(m,0x505e70u),offset=(datum&0xffffu)*0x54u;
    const auto candidate=read(m,pool+0x44u)+offset,restricted_unit=read(m,queue+0x7d8u);
    if(restricted_unit==none||read(m,candidate+4u)==restricted_unit||read(m,candidate+0x28u)==restricted_unit) {
        auto current=read(m,queue+0x7d0u),previous=none,next=none;
        while(current!=none) {
            const auto existing=row(m,current);postpone(m,candidate,existing,timing);
            const auto start=read(m,candidate+0x14u),existing_start=read(m,existing+0x14u);
            if(signed32(start)<=signed32(existing_start)){next=current;break;}
            previous=current;current=read(m,existing+0x50u);
        }
        const auto base=read(m,candidate+0x10u),start=read(m,candidate+0x14u);
        bool valid=signed32(start-base)<=word(m,candidate+0x1eu);
        if(valid&&read(m,candidate+0xeu,1)) {
            const auto dependency=read(m,candidate+0x44u);valid=dependency==none||dependency==previous;
        }
        if(valid) {
            auto previous_row=0u;if(previous!=none)previous_row=row(m,previous);
            write(m,previous_row?previous_row+0x50u:queue+0x7d0u,datum);
            write(m,candidate+0x50u,next);write(m,queue+0x7d4u,read(m,queue+0x7d4u,2)+1u,2);
            if(next!=none) {
                auto preceding=row(m,datum);const auto separation=read(m,0x46a16cu);
                timing.outcome.separation_loaded=true;timing.outcome.separation_bits=separation;
                const auto clock=read(m,0x51df54u);
                while(next!=none) {
                    const auto existing=row(m,next);const auto delay=timing.ticks(word(m,clock+2u),separation);
                    const auto preceding_start=read(m,preceding+0x14u),existing_start=read(m,existing+0x14u);
                    auto adjusted=existing_start;
                    if(signed32(existing_start)<signed32(preceding_start+delay)) {
                        const auto retry=timing.ticks(word(m,clock+2u),separation);adjusted=read(m,preceding+0x14u)+retry;
                    }
                    write(m,existing+0x14u,adjusted);postpone(m,existing,preceding,timing);
                    if(!read(m,existing+0x48u,1)) {
                        const auto actual_start=read(m,existing+0x14u),actual_base=read(m,existing+0x10u);
                        bool keep=signed32(actual_start-actual_base)<=word(m,existing+0x1eu);
                        if(keep&&read(m,existing+0xeu,1)) {
                            const auto dependency=read(m,existing+0x44u);keep=dependency==none||dependency==previous;
                        }
                        if(keep){preceding=existing;previous=next;}else native_campaign_ai_speech_queue_cancel_tu15(m,next);
                    }
                    next=read(m,existing+0x50u);
                }
            }
            admitted=true;
        }
    }
    native_campaign_ai_speech_queue_reclaim_tu15(m,queue);return {admitted,timing.outcome};
#else
    (void)m;(void)queue;(void)datum;(void)fp;(void)available;throw std::runtime_error("Native speech queue FP backend unavailable");
#endif
}
}
