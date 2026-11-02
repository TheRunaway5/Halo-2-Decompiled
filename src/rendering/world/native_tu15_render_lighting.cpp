#include "halo2/native_tu15_render_lighting.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);
    return value;
}
}
std::uint32_t native_tu15_object_body_bcdf0(const SourcePhysicsContactMemory& m,
    std::uint32_t object,std::uint32_t kind_mask) {
    if(object==0xffffffffu)return 0;
    const auto pool=read(m,0x4ecbd0u);
    const auto count=std::bit_cast<std::int32_t>(read(m,pool+0x38u));
    const auto index=object&65535u;
    if(std::int32_t(index)>=count)return 0;
    const auto stride=read(m,pool+0x24u),rows=read(m,pool+0x44u),header=rows+index*stride;
    const auto salt=read(m,header,2);
    if(!salt || salt!=(object>>16))return 0;
    const auto kind=read(m,header+3u,1);
    if(!(kind_mask&(1u<<(kind&31u))))return 0;
    return read(m,header+8u);
}
std::uint32_t native_tu15_render_lighting_2cfe0(const SourcePhysicsContactMemory& m,
    std::uint32_t object,std::uint32_t model,std::uint32_t instance,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15RenderLightingServices& services) {
    constexpr auto none=0xffffffffu;
    const auto fallback=[&] {
        if(model==none || object==none)return 3u;
        const auto body=services.object?services.object(object,fp):native_tu15_object_body_bcdf0(m,object,none);
        return body && (read(m,body+7u,1)&1u)?2u:3u;
    };
    const auto lightmaps=read(m,0x4ecc14u);
    if(!lightmaps || std::bit_cast<std::int32_t>(read(m,lightmaps+0x80u))<=0)return fallback();
    const auto bsp=read(m,0x4ecc18u);
    if(!bsp)return fallback();
    const auto group=read(m,lightmaps+0x84u);
    if(read(m,group+0x1cu)==none)return fallback();
    const auto checksum=read(m,group+4u);
    if(checksum!=read(m,bsp+8u))return fallback();
    if(instance==0x7ffu)return model==none?0u:2u;
    std::uint32_t references;
    if(model!=none) {
        const auto tags=read(m,0x4f0414u),definition=read(m,tags+16u*(model&65535u)+8u);
        const auto checksums=read(m,group+0x5cu),model_checksum=read(m,definition+8u);
        if(model_checksum!=read(m,checksums+12u*instance+8u))return 3u;
        references=read(m,group+0x64u);
    } else references=read(m,group+0x54u);
    const auto bucket=read(m,references+12u*instance+2u,2);
    const auto resources=read(m,group+0x44u),header=resources+56u*bucket+0xcu;
    if(!services.request)throw std::runtime_error("Native2CFE0 requires its genuine130980 resource owner");
    if(!services.request(header,fp))return 3u;
    if(model!=none)return 1u;
    const auto current_bsp=read(m,0x4ecc18u),instances=read(m,current_bsp+0x144u);
    return std::uint32_t(read(m,instances+88u*instance+0x56u,2)!=0);
}
}
