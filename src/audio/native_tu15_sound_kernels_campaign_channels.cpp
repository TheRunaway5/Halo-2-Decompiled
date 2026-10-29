// Closed semantic campaign audio leaves. The rest of the channel group is
// deliberately unregistered until its native dependency closure is proved.
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "halo2/native_campaign_audio_leaves.hpp"
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class AudioEnvironment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    unsigned host_mxcsr_{};
    NativeCampaignAudioFp& fp_;
public:
    explicit AudioEnvironment(NativeCampaignAudioFp& fp):fp_(fp) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native campaign fades require masked original FP exceptions");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());
        const std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);
        std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~AudioEnvironment() {
        asm volatile("fnstsw %0":"=a"(fp_.status));
        asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);
    }
};
__m128 scalar(std::uint32_t bits) {
    return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));
}
std::uint32_t bits(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
bool greater(__m128 left,__m128 right) {
    unsigned char result;
    asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(left),"x"(right):"cc","memory");
    return result!=0;
}
// Real host operations retain source NaN operand order; a compiler may commute
// an ordinary intrinsic ADD/MUL and select the other input's NaN payload.
__m128 add(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 sub(__m128 left,__m128 right){asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 mul(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 div(__m128 left,__m128 right){asm volatile("divss %1,%0":"+x"(left):"x"(right):"memory");return left;}
#endif
}
void native_campaign_advance_class_fades(NativeCampaignClassFades& bank,
    std::uint32_t elapsed_seconds_bits,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    AudioEnvironment environment(fp);
    const auto seconds=scalar(elapsed_seconds_bits);
    if(!greater(seconds,_mm_setzero_ps()))return;
    for(unsigned index=0;index<54;++index) {
        const bool interpolate=greater(scalar(bank.remaining_seconds(index)),seconds);
        const auto target=bank.target_gain(index);
        if(interpolate) {
            const auto current=scalar(bank.current_gain(index));
            const auto fraction=div(seconds,scalar(bank.remaining_seconds(index)));
            const auto difference=sub(scalar(target),current);
            const auto contribution=mul(difference,fraction);
            const auto remaining=scalar(bank.remaining_seconds(index));
            const auto gain=add(contribution,current);
            const auto time=sub(remaining,seconds);
            bank.current_gain(index,bits(gain),true);
            bank.remaining_seconds(index,bits(time),true);
        } else {
            bank.current_gain(index,target,false);
            bank.remaining_seconds(index,0u,false);
        }
    }
#else
    (void)bank;(void)elapsed_seconds_bits;(void)fp;
    throw std::runtime_error("Exact native campaign fade FP backend unavailable");
#endif
}
void native_campaign_build_voice_routes(NativeCampaignVoiceRoutes& bank) {
    for(std::int32_t word=0;word<24;++word)bank.word(word,0u,NativeCampaignRouteWrite::clear);
    const auto capacity_words=std::uint32_t((std::int32_t(bank.voice_capacity())+31)>>5)&0x3fffffffu;
    for(std::uint32_t word=0;word<capacity_words;++word)
        bank.word(std::int32_t(22u+word),0xffffffffu,NativeCampaignRouteWrite::available);
    for(std::int32_t voice=0;voice<bank.voice_count();++voice) {
        const auto flags=bank.voice_flags(unsigned(voice));
        const auto category=std::int32_t(flags&7u)-((flags&4u)?8:0);
        const auto word=category*4+(voice>>5);
        bank.word(word,bank.word(word)|(1u<<(voice&31)),NativeCampaignRouteWrite::category);
        const auto source=bank.source_voice(unsigned(voice));
        if(source!=255u && (bank.voice_flags(unsigned(voice))&7u)) {
            const auto signed_source=std::int32_t(std::bit_cast<std::int8_t>(source));
            const auto available=22+(signed_source>>5);
            bank.word(available,bank.word(available)&~(1u<<(signed_source&31)),NativeCampaignRouteWrite::claimed);
        }
    }
    for(unsigned index=0;index<3;++index) {
        bank.word(std::int32_t(16+index),std::uint32_t(std::int32_t(bank.selector(index))),NativeCampaignRouteWrite::first_selector);
        bank.word(std::int32_t(19+index),std::uint32_t(std::int32_t(bank.selector(index+3))),NativeCampaignRouteWrite::second_selector);
    }
}
} // namespace halo2

namespace halo2::native_tu15_sound {
// These adapters transport already admitted storage and scalar arguments.
// Neither reconstructs scratch registers or delegates algorithm work to a lift.
namespace {
struct FadeMemory final:NativeCampaignClassFades {
    CampaignContext& c;std::uint32_t base;
    FadeMemory(CampaignContext& context,std::uint32_t address):c(context),base(address){}
    std::uint32_t remaining_seconds(unsigned i)const override{return u32(c.m,base+i*16u+8u);}
    std::uint32_t target_gain(unsigned i)const override{return u32(c.m,base+i*16u);}
    std::uint32_t current_gain(unsigned i)const override{return u32(c.m,base+i*16u+4u);}
    void current_gain(unsigned i,std::uint32_t value,bool interpolated)override {
        store(c.m,c.o,interpolated?0x22836cu:0x228377u,base+i*16u+4u,value);
    }
    void remaining_seconds(unsigned i,std::uint32_t value,bool interpolated)override {
        store(c.m,c.o,interpolated?0x228371u:0x22837au,base+i*16u+8u,value);
    }
};
void advance_fades(CampaignContext& c,Abi& abi) {
    const auto input=abi.floating_point.value_or(c.fp);
    NativeCampaignAudioFp fp{input.x87_control_word,abi.x87_status,input.mxcsr};
    // Preserve the no-read gate: a nonpositive/unordered delta never reads the
    // global bank. The lazy bank resolves it only after the typed kernel gate.
    struct LazyFades final:NativeCampaignClassFades {
        CampaignContext& c;mutable std::optional<FadeMemory> memory;
        explicit LazyFades(CampaignContext& context):c(context){}
        FadeMemory& get()const {if(!memory)memory.emplace(c,u32(c.m,0x50ebf0u));return *memory;}
        std::uint32_t remaining_seconds(unsigned i)const override{return get().remaining_seconds(i);}
        std::uint32_t target_gain(unsigned i)const override{return get().target_gain(i);}
        std::uint32_t current_gain(unsigned i)const override{return get().current_gain(i);}
        void current_gain(unsigned i,std::uint32_t v,bool b)override{get().current_gain(i,v,b);}
        void remaining_seconds(unsigned i,std::uint32_t v,bool b)override{get().remaining_seconds(i,v,b);}
    } bank(c);
    native_campaign_advance_class_fades(bank,abi.xmm[1][0],fp);
    abi.floating_point=FP{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
    pop_return(c.m,abi);
}
struct RouteMemory final:NativeCampaignVoiceRoutes {
    CampaignContext& c;std::uint32_t source,output;
    RouteMemory(CampaignContext& context,std::uint32_t s,std::uint32_t d):c(context),source(s),output(d){}
    std::int16_t voice_capacity()const override{return std::bit_cast<std::int16_t>(std::uint16_t(u16(c.m,source+8u)));}
    std::int16_t voice_count()const override{return std::bit_cast<std::int16_t>(std::uint16_t(u16(c.m,source+4u)));}
    std::uint8_t voice_flags(unsigned i)const override{return u8(c.m,source+13u+i*0x34u);}
    std::uint8_t source_voice(unsigned i)const override{return u8(c.m,source+12u+i*0x34u);}
    std::int16_t selector(unsigned i)const override{return std::bit_cast<std::int16_t>(std::uint16_t(u16(c.m,source+0x1a0cu+i*2u)));}
    std::uint32_t word(std::int32_t i)const override{return u32(c.m,output+std::uint32_t(i)*4u);}
    void word(std::int32_t i,std::uint32_t value,NativeCampaignRouteWrite phase)override {
        constexpr std::uint32_t sites[]{0x225d87u,0x225da1u,0x225ddau,0x225e06u,0x225e26u,0x225e2cu};
        store(c.m,c.o,sites[unsigned(phase)],output+std::uint32_t(i)*4u,value);
    }
};
void build_routes(CampaignContext& c,Abi& abi) {
    if(abi.eflags&0x400u)throw FormatError("Native campaign voice routes require the compiled caller direction flag");
    RouteMemory bank(c,u32(c.m,0x52d674u),argument(c.m,abi,0));
    native_campaign_build_voice_routes(bank);pop_return(c.m,abi,4);
}
}
std::span<const CampaignRoutineEntry> campaign_channel_routines() {
    static constexpr CampaignRoutineEntry entries[]{{0x228310u,advance_fades,true},{0x225d70u,build_routes,true}};
    return entries;
}
} // namespace halo2::native_tu15_sound
