#include "halo2/native_campaign_ai_speech_cluster_mask_tu15.hpp"
#include "halo2/native_campaign_audio_listeners.hpp"
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
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
struct Bytes {
    const SourcePhysicsContactMemory &memory;std::uint32_t address;
    std::uint8_t at(std::uint32_t index)const{return std::uint8_t(read(memory,address+index,1));}
};
struct Scale {
    const SourcePhysicsContactMemory &memory;std::uint32_t address;
    Bytes bytes()const{return {memory,read(memory,address+0x28u)};}
    std::uint32_t lower()const{return read(memory,address+4u);}
    std::uint32_t upper()const{return read(memory,address+8u);}
};
struct Structure {
    const SourcePhysicsContactMemory &memory;std::uint32_t address;
    std::uint32_t cluster_count()const{return read(memory,address+0x9cu);}
    std::int32_t scale_count()const{return signed32(read(memory,address+0x224u));}
    Bytes cluster_bytes()const{return {memory,read(memory,address+0xe8u)};}
    Scale scale_table()const{return {memory,read(memory,address+0x228u)};}
};
class SimdEnvironment {
    NativeCampaignAudioFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeCampaignAudioFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native speech cluster mask requires masked SIMD traps");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
}
bool native_campaign_ai_speech_cluster_blocked_tu15(const SourcePhysicsContactMemory &m,
    std::uint32_t structure,std::int32_t first,std::int32_t second) {
    if(first==second)return false;
    Structure source{m,structure};
    if(source.scale_count()<=0)return (native_campaign_cluster_byte(source,std::int16_t(first),std::int16_t(second))>>7u)!=0;
    const auto scale=source.scale_table();const auto flags=read(m,scale.address+0x20u);
    const auto low=std::uint32_t(first<second?first:second),high=std::uint32_t(first<second?second:first);
    const auto count=source.cluster_count();
    const auto index=native_campaign_listeners_detail::pair_offset(low,high,count);
    const auto word=flags+std::uint32_t(signed32(index)>>5)*4u;
    return (read(m,word)&(1u<<(index&31u)))!=0;
}
std::uint32_t native_campaign_ai_speech_cluster_mask_tu15(SourcePhysicsContactMemory &m,
    std::int16_t cluster,NativeCampaignAudioFp &fp) {
    constexpr std::uint32_t bitmap=0x502200u,key=0x502240u;
    if(std::uint16_t(cluster)==read(m,key,2))return bitmap;
    const auto address=read(m,0x4ecc18u);Structure structure{m,address};
    const auto rounded=std::uint32_t(signed32(structure.cluster_count()+31u)>>5)*4u;
    for(std::uint32_t n=0;n<(rounded>>2u);++n)write(m,bitmap+n*4u,0);
    if(signed32(structure.cluster_count())>0) {
        const auto threshold_bits=read(m,0x46a1a8u);SimdEnvironment environment(fp);
        std::uint32_t ordinal=0;
        for(;;) {
            const auto candidate=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(ordinal)));
            if(!native_campaign_ai_speech_cluster_blocked_tu15(m,address,cluster,candidate)) {
                const auto scale=native_campaign_listeners_detail::cluster_scale(structure,cluster,candidate);
                if(native_campaign_ducking_detail::above(native_campaign_ducking_detail::scalar(threshold_bits),native_campaign_ducking_detail::scalar(scale))) {
                    const auto at=bitmap+std::uint32_t(candidate>>5)*4u;
                    write(m,at,read(m,at)|(1u<<(std::uint32_t(candidate)&31u)));
                }
            }
            const auto count=structure.cluster_count();++ordinal;
            const auto next=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(ordinal)));
            if(next>=signed32(count))break;
        }
    }
    write(m,key,std::uint16_t(cluster),2);return bitmap;
}
}
