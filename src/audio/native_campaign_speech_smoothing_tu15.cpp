#include "halo2/native_campaign_speech_smoothing_tu15.hpp"
#include "halo2/native_campaign_audio_stop_callback.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

namespace halo2 { namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
    return value;
}
struct Header {
    const SourcePhysicsContactMemory &memory;std::uint32_t address;
    bool present()const{return address!=0;}
    std::int16_t salt()const{return std::bit_cast<std::int16_t>(std::uint16_t(read(memory,address,2)));}
    std::uint8_t type()const{return std::uint8_t(read(memory,address+3u,1));}
    std::uint32_t body()const{return read(memory,address+8u);}
};
struct Objects {
    const SourcePhysicsContactMemory &memory;std::uint32_t address;
    std::int32_t extent()const{return std::bit_cast<std::int32_t>(read(memory,address+0x38u));}
    Header indexed(std::uint32_t index)const {
        const auto stride=read(memory,address+0x24u);
        const auto rows=read(memory,address+0x44u);
        return {memory,rows+index*stride};
    }
};
struct State {
    const SourcePhysicsContactMemory &memory;
    Objects objects()const{return {memory,read(memory,0x4ecbd0u)};}
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
__m128 scalar(std::uint32_t bits) {
    return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));
}
std::uint32_t bits(__m128 value) {
    return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));
}
class SimdEnvironment {
    NativeCampaignAudioFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeCampaignAudioFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native speech smoothing requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
#endif
}
namespace {
void update_into(SourcePhysicsContactMemory &memory,std::uint32_t object,
    std::uint32_t sampled_bits,NativeCampaignAudioFp &fp,
    const SourcePhysicsContactRecordObserver &observer,NativeCampaignSpeechSmoothingProgressTu15 *progress) {
    const auto state=read(memory,0x4f9b5cu);
    if(!read(memory,state+0x29u,1))return;
    // Reuse the qualified generic BCDF0 owner with precisely its mask3 call.
    // Its result is only an admission test: original18F7BC refreshes the pool.
    State lookup{memory};if(!native_campaign_sound_object(lookup,object,3u))return;
    const auto objects=read(memory,0x4ecbd0u);
    const auto rows=read(memory,objects+0x44u);
    const auto lower_bits=read(memory,0x46a468u);
    if(progress)progress->lower_bound=lower_bits;
    const auto body=read(memory,rows+(object&0xffffu)*12u+8u);
    const auto previous_bits=read(memory,body+0x208u);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);
    auto delta=scalar(sampled_bits),previous=scalar(previous_bits),lower=scalar(lower_bits);
    asm volatile("subss %1,%0":"+x"(delta):"x"(previous):"memory");
    if(progress)progress->difference=bits(delta);
    unsigned char below{};
    asm volatile("comiss %2,%1; seta %0":"=qm"(below):"x"(lower),"x"(delta):"cc","memory");
    bool selected_bound=below;
    if(below)delta=lower;
    else {
        const auto upper_bits=read(memory,0x456918u);
        if(progress)progress->upper_bound=upper_bits;
        auto upper=scalar(upper_bits);unsigned char within{};
        asm volatile("comiss %2,%1; setbe %0":"=qm"(within):"x"(delta),"x"(upper):"cc","memory");
        if(!within){delta=upper;selected_bound=true;}
    }
    if(progress)progress->selected_delta=NativeCampaignSpeechSmoothingDeltaTu15{bits(delta),selected_bound};
    const auto current_bits=read(memory,body+0x208u);
    if(progress)progress->current_level=current_bits;
    auto current=scalar(current_bits);
    asm volatile("addss %1,%0":"+x"(current):"x"(delta):"memory");
    const auto value=bits(current);
    if(progress)progress->updated_level=value;
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
    if(!observer){memory.write(body+0x208u,bytes);return;}
    SourcePhysicsContactRecordStore event{0x18f804u,body+0x208u,{},std::vector<std::byte>(bytes.begin(),bytes.end())};
    event.before.resize(4);memory.read(event.address,event.before);
    memory.write(body+0x208u,bytes);
    observer(event);
#else
    (void)sampled_bits;(void)previous_bits;(void)fp;(void)observer;
    throw std::runtime_error("Exact native speech smoothing backend unavailable");
#endif
}
}
NativeCampaignSpeechSmoothingProgressTu15 native_campaign_speech_smoothing_tu15(SourcePhysicsContactMemory &memory,std::uint32_t object,
    std::uint32_t sampled_bits,NativeCampaignAudioFp &fp,
    const SourcePhysicsContactRecordObserver &observer) {
    NativeCampaignSpeechSmoothingProgressTu15 progress;
    try {update_into(memory,object,sampled_bits,fp,observer,&progress);return progress;}
    catch(...){throw NativeCampaignSpeechSmoothingPendingErrorTu15(std::move(progress),fp,std::current_exception());}
}
}
