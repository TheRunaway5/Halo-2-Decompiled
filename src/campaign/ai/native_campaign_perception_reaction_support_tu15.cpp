#include "halo2/native_campaign_perception_reaction_support_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned width=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
void write16(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint16_t value) {
    const std::array<std::byte,2> bytes{std::byte(value),std::byte(value>>8u)};m.write(p,bytes);
}
std::uint32_t walk(const SourcePhysicsContactMemory &m,std::uint32_t object,std::uint32_t rows) {
    auto root=none;
    while(object!=none){root=object;const auto record=read(m,rows+(object&0xffffu)*12u+8u);object=read(m,record+0x14u);}
    return root;
}
std::uint32_t state_record(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    const auto objects=read(m,0x4ecbd0u);
    auto root=none;
    if(object!=none){const auto rows=read(m,objects+0x44u);root=walk(m,object,rows);}
    const auto rows=read(m,objects+0x44u);return read(m,rows+(root&0xffffu)*12u+8u);
}
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::runtime_error("Native transition plane requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
__m128 load(const SourcePhysicsContactMemory &m,std::uint32_t p){return scalar(read(m,p));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(__m128 a,__m128 b){unsigned char result{};asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
}
NativeCampaignPerceptionTransitionTu15 native_campaign_perception_transition_value_tu15(
    const SourcePhysicsContactMemory &m,std::uint32_t location,std::uint32_t point,NativeTu15PhysicsFrameFp &fp) {
    const auto cluster=std::uint16_t(read(m,location+4u,2));if(cluster==0xffffu)return {};
    const auto bsp=read(m,0x4ecc18u),clusters=read(m,bsp+0xa0u);
    const auto signed_cluster=std::int32_t(std::bit_cast<std::int16_t>(cluster));
    const auto selector=read(m,clusters+std::uint32_t(signed_cluster)*0xb0u+0x70u,1);
    if(selector==0xffu)return {};
    const auto planes=read(m,bsp+0x68u),plane=planes+(selector&0x7fu)*24u;
    const auto sector=std::uint16_t(read(m,plane+2u,2));if(sector==0xffffu)return {};
    if(selector&0x80u) {
        const auto nz=load(m,plane+12u),z=load(m,point+8u);
        SimdEnvironment environment(fp);auto distance=mul(nz,z);
        const auto ny=load(m,plane+8u),y=load(m,point+4u);const auto dy=mul(ny,y);distance=add(distance,dy);
        const auto nx=load(m,plane+4u),x=load(m,point);const auto dx=mul(nx,x);distance=add(distance,dx);
        distance=sub(distance,load(m,plane+16u));
        if(!above(scalar(0),distance))return {};
    }
    return {true,sector};
}
bool native_campaign_perception_transition_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t location,std::uint32_t point,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp) {
    const auto result=native_campaign_perception_transition_value_tu15(m,location,point,fp);
    if(result.accepted && destination)write16(m,destination,*result.sector);
    return result.accepted;
}
std::uint32_t native_campaign_perception_flagged_root_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    if(object==none)return none;
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u);
    while(true) {
        const auto record=read(m,rows+(object&0xffffu)*12u+8u),flags=read(m,record+4u);
        if(!(flags&0x4000000u))return object;
        object=read(m,record+0x14u);if(object==none)return none;
    }
}
std::uint32_t native_campaign_perception_vehicle_root_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    const auto root=native_campaign_perception_flagged_root_tu15(m,object);
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u);
    return read(m,rows+(root&0xffffu)*12u+3u,1)==1u?root:object;
}
std::uint32_t native_campaign_perception_parent_root_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    if(object==none)return none;
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u);return walk(m,object,rows);
}
bool native_campaign_perception_definition_bit7_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),record=read(m,rows+(object&0xffffu)*12u+8u);
    const auto tag=read(m,record),tags=read(m,0x4f0414u),definition=read(m,tags+(tag&0xffffu)*16u+8u);
    return (read(m,definition+0x1ecu)&0x80u)!=0;
}
bool native_campaign_perception_root_state_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    const auto record=state_record(m,object),kind=read(m,record+0xaau,1);
    if(kind==12u){if(read(m,record+0x17cu,1)==1u)return !(read(m,record+0x12cu,1)&0x80u);return true;}
    if(kind!=0u)return false;
    if(read(m,record+0x354u)!=none)return true;
    const auto previous=read(m,record+0x350u),clock=read(m,0x51df54u);return previous==read(m,clock+8u);
}
bool native_campaign_perception_root_cacheable_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    const auto record=state_record(m,object),kind=read(m,record+0xaau,1);
    if(kind==12u){if(read(m,record+0x17cu,1)==1u)return !(read(m,record+0x12cu,1)&0x80u);return true;}
    if(kind==0u && read(m,record+0x3dcu,1)==1u)return !(read(m,record+0x348u,1)&1u);
    return true;
}
}
