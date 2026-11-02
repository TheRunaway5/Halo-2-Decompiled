#include "halo2/native_tu15_activation_payload_filter.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t address,unsigned width=4){
    std::array<std::byte,4>bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
}
std::uint32_t native_tu15_activation_body_datum(const SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsBody body){
    const auto count=word(memory,body.identity+0x34u);std::uint32_t index{};
    if(std::bit_cast<std::int32_t>(count)>0){
        const auto properties=word(memory,body.identity+0x30u);
        do{
            if(word(memory,properties+index*8u)==0x2001u)return word(memory,properties+index*8u+4u);
            ++index;
        }while(std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(count));
    }
    return 0;
}
std::uint32_t native_tu15_activation_payload_filter(SourcePhysicsContactMemory&memory,
 NativeTu15ContactPayload first,NativeTu15ContactPayload second,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15ActivationPayloadFilterServices&services,NativeTu15ActivationPayloadFilterStats*stats){
    if(stats)++stats->calls;
    const auto first_body=word(memory,first.identity+0x18u)==1u?word(memory,first.identity+0x20u):0u;
    const auto second_body=word(memory,second.identity+0x18u)==1u?word(memory,second.identity+0x20u):0u;
    if(!first_body||!second_body)return 0;
    if(word(memory,first_body+0x40u,1))return 1;
    if(word(memory,second_body+0x40u,1))return 1;
    const auto motion_kind=[&](std::uint32_t body){
        const auto motion=word(memory,body+0x3cu),table=word(memory,motion),owner=word(memory,table+0x18u);
        if(stats)++stats->motion_queries;
        if(const auto kind=native_tu15_activation_type(owner)){
            if(stats)++stats->native_motion_queries;return *kind;
        }
        if(stats)++stats->unconverted_motion_queries;
        if(!services.motion_kind)throw std::runtime_error("Native TU1.5 activation payload filter requires its actual motion-type owner");
        return services.motion_kind(motion,{owner},fp);
    };
    if(motion_kind(first_body)==6u)return 1;
    if(motion_kind(second_body)==6u)return 1;
    if(stats)++stats->property_queries;
    const auto first_datum=native_tu15_activation_body_datum(memory,{first_body});
    if(first_datum==0xffffffffu)return 0;
    if(stats)++stats->property_queries;
    if(native_tu15_activation_body_datum(memory,{second_body})==0xffffffffu)return 0;
    // 281F9A..281FCB intentionally selects the first datum's component and
    // object. The second property result was tested above, then overwritten.
    const auto manager=word(memory,0x52be5cu),components=word(memory,manager+0x44u),
        objects=word(memory,0x4ecbd0u),object=word(memory,components+(first_datum&0xffffu)*160u+8u),
        headers=word(memory,objects+0x44u),header=headers+(object&0xffffu)*12u;
    const auto type=word(memory,header+3u,1);
    if(type==1u){
        if(type!=word(memory,header+3u,1))return 1;
    }else{
        if(word(memory,header+3u,1)==1u)
            if(type!=word(memory,header+3u,1))return 1;
        return 1;
    }
    return 0;
}
} // namespace halo2
