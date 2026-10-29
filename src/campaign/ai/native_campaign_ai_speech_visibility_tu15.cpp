#include "halo2/native_campaign_ai_speech_visibility_tu15.hpp"
#include "halo2/native_campaign_ai_perception_policy_tu15.hpp"
#include "halo2/native_campaign_ai_speech_cluster_mask_tu15.hpp"
#include "halo2/native_campaign_audio_listeners.hpp"
#include <bit>
#include <optional>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(i*8u);return value;
}
std::int32_t signed32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::int16_t read_word(const SourcePhysicsContactMemory &m,std::uint32_t p){return std::bit_cast<std::int16_t>(std::uint16_t(read(m,p,2)));}
struct RawPoint {
    const SourcePhysicsContactMemory &memory;std::uint32_t address;
    std::uint32_t scalar(unsigned offset)const{return read(memory,address+offset);}
    std::int16_t word(unsigned offset)const{return read_word(memory,address+offset);}
};
struct ValuePoint {
    std::span<const std::uint32_t> values;
    std::uint32_t scalar(unsigned offset)const{return values[offset/4u];}
    std::int16_t word(unsigned offset)const{return std::bit_cast<std::int16_t>(std::uint16_t(values[offset/4u]>>((offset&3u)*8u)));}
};
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
using namespace native_campaign_ducking_detail;
std::pair<std::uint32_t,std::uint32_t> metric(std::uint32_t squared,std::uint32_t cluster,std::uint32_t factor) {
    auto scaled=scalar(cluster);const auto multiplier=scalar(factor);std::uint32_t length;
    // The original extended square root stays live across the independent
    // single-precision cluster multiplication, before its rounded store.
    asm volatile("flds %2; fsqrt; mulss %3,%1; fstps %0":"=m"(length),"+x"(scaled):"m"(squared),"x"(multiplier):"memory","st");
    return {bits(scaled),length};
}
template<class Target,class Observer> std::uint16_t visible(const SourcePhysicsContactMemory &m,
    std::uint32_t actor,const Target &target,const Observer &observer,std::uint32_t location,
    std::int16_t alert,std::int16_t mode,NativeCampaignAudioFp &fp,unsigned available) {
    const auto actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u),record=actor_rows+(actor&0xffffu)*0x888u;
    const auto policy=native_campaign_ai_character_perception_policy_tu15(m,actor);
    if(!policy||alert==0)return 0;
    const auto observer_cluster=observer.word(0x28u);if(observer_cluster==-1)return 0;
    const auto target_cluster=read_word(m,location+4u);if(target_cluster==-1)return 0;
    const auto target_x=scalar(target.scalar(0)),observer_x=scalar(observer.scalar(0));
    Environment environment(fp);const auto dx=sub(target_x,observer_x);
    const auto target_z=scalar(target.scalar(8)),observer_z=scalar(observer.scalar(8)),dz=sub(target_z,observer_z);
    const auto target_y=scalar(target.scalar(4)),observer_y=scalar(observer.scalar(4)),dy=sub(target_y,observer_y);
    auto radius=scalar(read(m,policy+0x18u));
    const auto zz=mul(dz,dz),xx=mul(dx,dx);auto squared=add(zz,xx);const auto yy=mul(dy,dy);squared=add(squared,yy);
    const auto facing_z=mul(scalar(observer.scalar(0x20u)),dz),facing_y=mul(scalar(observer.scalar(0x1cu)),dy),facing_x=mul(scalar(observer.scalar(0x18u)),dx);
    const auto facing=add(add(facing_z,facing_y),facing_x);
    if(above(_mm_setzero_ps(),facing))radius=mul(radius,scalar(read(m,0x456910u)));
    const auto posture=read_word(m,record+0x84u);const auto half=read(m,0x46a22cu);
    if(posture==3)radius=mul(radius,scalar(half));
    else if(posture==1)radius=mul(radius,scalar(read(m,0x46a240u)));
    if(alert==4)radius=mul(radius,scalar(read(m,0x46a16cu)));
    else if(alert==1)radius=mul(radius,scalar(read(m,0x46a330u)));
    else if(alert==3)radius=mul(radius,scalar(half));
    if(native_campaign_ai_speech_cluster_environment_tu15(m,observer.word(0x28u))||native_campaign_ai_speech_location_environment_tu15(m,location))radius=mul(radius,scalar(read(m,0x46a130u)));
    if(mode!=0&&mode!=1)radius=mul(radius,scalar(half));
    if(!above(mul(radius,radius),squared))return 0;
    const auto structure=read(m,0x4ecc18u);
    if(native_campaign_ai_speech_cluster_blocked_tu15(m,structure,target_cluster,observer_cluster))return 0;
    const auto scale=native_campaign_listeners_detail::cluster_scale(Structure{m,structure},target_cluster,observer_cluster);
    if(available<1)throw std::runtime_error("Native speech visibility requires one available extended temporary");
    const auto factor=read(m,0x46a514u);const auto [cluster_distance,length]=metric(bits(squared),scale,factor);
    const auto effective=above(scalar(cluster_distance),scalar(length))?scalar(cluster_distance):scalar(length);
    if(!above(radius,effective))return 0;
    return alert>=3?3u:2u;
}
}
bool native_campaign_ai_speech_cluster_environment_tu15(const SourcePhysicsContactMemory &m,std::int16_t cluster) {
    const auto structure=read(m,0x4ecc18u),clusters=read(m,structure+0xa0u),record=clusters+std::uint32_t(std::int32_t(cluster))*0xb0u;
    const auto environment=read_word(m,record+0x72u);if(environment==-1)return false;
    if(std::int32_t(environment)>=signed32(read(m,structure+0xd4u)))return false;
    const auto environments=read(m,structure+0xd8u),tag=read(m,environments+std::uint32_t(std::int32_t(environment))*0x64u+0x24u);
    if(tag==0xffffffffu)return false;
    const auto tags=read(m,0x4f0414u),definition=read(m,tags+(tag&0xffffu)*16u+8u);
    return (read(m,definition,1)&1u)!=0;
}
bool native_campaign_ai_speech_location_environment_tu15(const SourcePhysicsContactMemory &m,std::uint32_t location) {
    return native_campaign_ai_speech_cluster_environment_tu15(m,read_word(m,location+4u));
}
std::uint16_t native_campaign_ai_speech_visibility_tu15(const SourcePhysicsContactMemory &m,std::uint32_t actor,
    std::uint32_t target,std::uint32_t observer,std::uint32_t location,std::int16_t alert,std::int16_t mode,
    NativeCampaignAudioFp &fp,unsigned available) {
    return visible(m,actor,RawPoint{m,target},RawPoint{m,observer},location,alert,mode,fp,available);
}
std::uint16_t native_campaign_ai_speech_visibility_value_tu15(const SourcePhysicsContactMemory &m,std::uint32_t actor,
    std::span<const std::uint32_t,3> target,std::span<const std::uint32_t,14> observer,std::uint32_t location,
    std::int16_t alert,std::int16_t mode,NativeCampaignAudioFp &fp,unsigned available) {
    return visible(m,actor,ValuePoint{target},ValuePoint{observer},location,alert,mode,fp,available);
}
}
