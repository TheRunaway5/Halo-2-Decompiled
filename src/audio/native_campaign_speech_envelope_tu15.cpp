#include "halo2/native_campaign_speech_envelope_tu15.hpp"
#include <array>
#include <bit>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignSpeechEnvelopePendingErrorTu15::NativeCampaignSpeechEnvelopePendingErrorTu15(
    std::uint32_t sample,std::exception_ptr cause):
    std::runtime_error("Speech envelope has a pending converted byte sample"),sample_(sample),cause_(std::move(cause)){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
std::int32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(memory,address,2)));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeCampaignAudioFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeCampaignAudioFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native speech envelope requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
std::uint32_t bits(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
#endif
}
NativeCampaignSpeechEnvelopeSampleTu15 native_campaign_speech_envelope_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t sound,std::int16_t range,
    std::int16_t permutation,std::int16_t sample,NativeCampaignAudioFp &fp) {
    const auto tags=read(memory,0x4f0414u),definition=read(memory,tags+(sound&0xffffu)*16u+8u);
    const auto range_index=word(memory,definition+8u)+std::int32_t(range);
    const auto gestalt=read(memory,0x52d664u),ranges=read(memory,gestalt+0x24u);
    const auto range_record=ranges+std::uint32_t(range_index)*12u;
    std::int32_t selected{};
    if(permutation>=0) {
        const auto last=word(memory,range_record+0xau)-1;
        selected=std::int32_t(permutation)<=last?std::int32_t(permutation):last;
    }
    if(selected!=permutation)return {false,0};
    const auto resource_index=word(memory,definition+0xeu),first=word(memory,range_record+8u);
    const auto resources=read(memory,gestalt+0x54u),permutations=read(memory,gestalt+0x2cu);
    const auto resource=read(memory,resources+std::uint32_t(resource_index)*0x2cu+4u);
    const auto curves=read(memory,resource+0xcu);
    const auto curve_index=std::bit_cast<std::int8_t>(std::uint8_t(read(memory,permutations+std::uint32_t(first+permutation)*16u+5u,1)));
    const auto curve=curves+std::uint32_t(std::int32_t(curve_index))*16u;
    const auto count=std::bit_cast<std::int32_t>(read(memory,curve+4u));
    if(count<=0)return {false,0};
    const auto sample_index=sample<0?0:(std::int32_t(sample)>count-1?count-1:std::int32_t(sample));
    const auto bytes=read(memory,resource+4u),offset=read(memory,curve);
    const auto value=read(memory,bytes+offset+std::uint32_t(sample_index),1);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);
    auto converted=_mm_cvtsi32_ss(_mm_setzero_ps(),std::int32_t(value));
    std::uint32_t scale;
    try {scale=read(memory,0x46a0c4u);}
    catch(...){throw NativeCampaignSpeechEnvelopePendingErrorTu15(bits(converted),std::current_exception());}
    const auto multiplier=scalar(scale);asm volatile("mulss %1,%0":"+x"(converted):"x"(multiplier):"memory");
    return {true,bits(converted)};
#else
    (void)value;(void)fp;throw std::runtime_error("Exact native speech envelope backend unavailable");
#endif
}
}
