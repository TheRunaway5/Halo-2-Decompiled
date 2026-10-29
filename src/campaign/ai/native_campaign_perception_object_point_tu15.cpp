#include "halo2/native_campaign_perception_object_point_tu15.hpp"
#include "halo2/native_campaign_object_marker_request_tu15.hpp"
#include "halo2/native_campaign_object_motion_refs_tu15.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include <array>
#include <bit>
#include <optional>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned width=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));m.write(p,bytes);
}
void copy_vector(SourcePhysicsContactMemory &m,std::uint32_t source,std::uint32_t destination) {
    for(unsigned lane=0;lane<3;++lane)write(m,destination+lane*4u,read(m,source+lane*4u));
}
void publish_marker_point(SourcePhysicsContactMemory &m,std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp) {
    const auto marker=native_campaign_object_marker_request_value_tu15(m,object,false,0x4000095u,fp);
    for(unsigned lane=0;lane<3;++lane)write(m,destination+lane*4u,marker.value.world[10u+lane]);
}
bool unit_kind(std::uint32_t kind){return ((1u<<(kind&31u))&3u)!=0;}
std::uint32_t root_object(std::uint32_t object,const SourcePhysicsContactMemory &m,std::uint32_t rows) {
    auto root=none;
    while(object!=none){root=object;const auto record=read(m,rows+(object&0xffffu)*12u+8u);object=read(m,record+0x14u);}
    return root;
}
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::runtime_error("Native attachment selection requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(__m128 a,__m128 b){unsigned char value{};asm volatile("comiss %2,%1\n\tseta %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value;}
}
void native_campaign_perception_object_point_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t object,std::uint32_t out,NativeTu15PhysicsFrameFp &fp) {
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),record=read(m,rows+(object&0xffffu)*12u+8u);
    native_campaign_object_world_point_tu15(m,object,out+12u,fp);
    copy_vector(m,record+0x70u,out+0x18u);
    if(unit_kind(read(m,record+0xaau,1)))publish_marker_point(m,object,out,fp);
    else {
        const auto x=read(m,out+12u),y=read(m,out+16u);write(m,out,x);
        const auto z=read(m,out+20u);write(m,out+4u,y);write(m,out+8u,z);
    }
    native_campaign_object_motion_refs_tu15(m,object,out+0x2cu,0);
    auto root=none;
    if(object!=none){const auto current_objects=read(m,0x4ecbd0u),current_rows=read(m,current_objects+0x44u);root=root_object(object,m,current_rows);}
    const auto current_objects=read(m,0x4ecbd0u),current_rows=read(m,current_objects+0x44u),current=read(m,current_rows+(root&0xffffu)*12u+8u);
    write(m,out+0x24u,read(m,current+0x28u));write(m,out+0x28u,read(m,current+0x2cu));
}
void native_campaign_perception_attachment_point_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t object,std::uint32_t out,NativeTu15PhysicsFrameFp &fp) {
    const auto marker=native_campaign_object_marker_request_value_tu15(m,object,false,0x4000095u,fp);
    if(std::bit_cast<std::int16_t>(marker.count)>0) {
        for(unsigned lane=0;lane<3;++lane)write(m,out+lane*4u,marker.value.world[10u+lane]);
    }else {
        const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),record=read(m,rows+(object&0xffffu)*12u+8u);
        copy_vector(m,record+0x30u,out);
    }
}
void native_campaign_perception_object_state_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t object,std::uint32_t out,NativeTu15PhysicsFrameFp &fp) {
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),record=read(m,rows+(object&0xffffu)*12u+8u);
    const auto root=root_object(object,m,rows),root_record=read(m,rows+(root&0xffffu)*12u+8u);
    native_campaign_object_world_point_tu15(m,object,out+12u,fp);
    copy_vector(m,root_record+0x88u,out+0x2cu);
    write(m,out+0x24u,read(m,root_record+0x28u));write(m,out+0x28u,read(m,root_record+0x2cu));
    const auto kind=read(m,record+0xaau,1);
    if(unit_kind(kind))publish_marker_point(m,object,out,fp);
    else if(kind==12u)native_campaign_perception_attachment_point_tu15(m,object,out,fp);
    else {
        const auto x=read(m,out+12u),y=read(m,out+16u),z=read(m,out+20u);
        write(m,out,x);write(m,out+4u,y);write(m,out+8u,z);
    }
}
bool native_campaign_perception_cluster_point_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t cluster,std::uint32_t point,std::uint32_t out,NativeTu15PhysicsFrameFp &fp) {
    auto best_distance=scalar(read(m,0x45695cu));
    const auto objects=read(m,0x4ecbd0u),clusters=read(m,0x510fa8u),rows=read(m,clusters+0x44u),record=rows+(cluster&0xffffu)*0x34u;
    auto object=read(m,record+0x18u),best=none;
    std::optional<SimdEnvironment> environment;
    while(object!=none) {
        const auto object_rows=read(m,objects+0x44u),current=read(m,object_rows+(object&0xffffu)*12u+8u),state=read(m,current+0x134u);
        auto next=none;
        if(!state){const auto offset=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,current+0x13au,2))));const auto extension=current+std::uint32_t(offset);if(extension)next=read(m,extension+12u);}
        std::uint32_t extension{};
        if(!state){const auto offset=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,current+0x13au,2))));extension=current+std::uint32_t(offset);}
        const auto query_y=scalar(read(m,point+4u)),object_y=scalar(read(m,extension+0x18u));
        if(!environment)environment.emplace(fp);
        const auto dy=sub(query_y,object_y);
        const auto query_x=scalar(read(m,point)),object_x=scalar(read(m,extension+0x14u));const auto dx=sub(query_x,object_x);
        const auto query_z=scalar(read(m,point+8u)),object_z=scalar(read(m,extension+0x1cu));const auto dz=sub(query_z,object_z);
        const auto yy=mul(dy,dy),xx=mul(dx,dx);auto distance=add(xx,yy);const auto zz=mul(dz,dz);distance=add(distance,zz);
        if(above(best_distance,distance)){best_distance=distance;best=object;}
        object=next;
    }
    environment.reset();
    if(best==none)return false;
    native_campaign_perception_object_point_tu15(m,best,out,fp);return true;
}
bool native_campaign_perception_actor_point_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t actor,std::uint32_t point,std::uint32_t out,NativeTu15PhysicsFrameFp &fp) {
    const auto actors=read(m,0x5020c8u),rows=read(m,actors+0x44u),record=rows+(actor&0xffffu)*0x888u;
    if(read(m,record+7u,1)){const auto cluster=read(m,record+0x1cu);if(cluster!=none)return native_campaign_perception_cluster_point_tu15(m,cluster,point,out,fp);}
    for(unsigned word=0;word<14;++word)write(m,out+word*4u,read(m,record+0x22cu+word*4u));
    return true;
}
}
