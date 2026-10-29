#include "halo2/native_campaign_object_motion_refs_tu15.hpp"
#include <array>
#include <bit>
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
std::uint32_t object_record(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u);return read(m,rows+(object&0xffffu)*12u+8u);
}
}
bool native_campaign_object_pickup_parent_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    const auto record=object_record(m,object),flags=read(m,record+0x12cu,1);
    return (flags&1u) && !(flags&2u);
}
std::uint32_t native_campaign_object_marker_owner_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    while(true) {
        const auto record=object_record(m,object),parent=read(m,record+0x14u);
        if(parent!=none) {
            const auto tag=read(m,record),tags=read(m,0x4f0414u),definition=read(m,tags+(tag&0xffffu)*16u+8u);
            if(read(m,definition+2u,1)&0x10u){object=parent;continue;}
        }
        if(read(m,record+4u,1)&1u) {
            const auto kind=read(m,record+0xaau,1);
            if(((1u<<(kind&31u))&0x1cu) && native_campaign_object_pickup_parent_tu15(m,object))return read(m,record+0x154u);
        }
        return object;
    }
}
void native_campaign_object_motion_component_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t component,std::uint32_t index,std::uint32_t destination,bool angular) {
    const auto rows=read(m,component+0x70u),body=read(m,rows+index*0x60u+0x40u);
    if(!read(m,body+0x40u,1)) {
        const auto motion=read(m,body+0x3cu),source=motion+(angular?0x50u:0x40u);
        const auto z=read(m,source+8u),y=read(m,source+4u),x=read(m,source);
        write(m,destination,x);write(m,destination+4u,y);write(m,destination+8u,z);
    }else copy_vector(m,read(m,0x474d04u),destination);
}
void native_campaign_object_motion_refs_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t object,std::uint32_t linear,std::uint32_t angular) {
    auto root=none;const auto objects=read(m,0x4ecbd0u);
    if(object!=none) {
        const auto rows=read(m,objects+0x44u);
        do{root=object;const auto record=read(m,rows+(object&0xffffu)*12u+8u);object=read(m,record+0x14u);}while(object!=none);
    }
    const auto rows=read(m,objects+0x44u),record=read(m,rows+(root&0xffffu)*12u+8u),component_datum=read(m,record+0xb4u);
    if(component_datum!=none) {
        const auto components=read(m,0x52be5cu),component_rows=read(m,components+0x44u),component=component_rows+(component_datum&0xffffu)*0xa0u;
        const auto active_index=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(m,component+0x18u,1))));
        if(active_index>=0 && active_index<std::bit_cast<std::int32_t>(read(m,component+0x74u))) {
            const auto flags=read(m,record+0xc1u,1),zero=read(m,0x474d04u);
            if(linear) {
                if(flags&1u)copy_vector(m,zero,linear);
                else native_campaign_object_motion_component_tu15(m,component,std::uint32_t(active_index),linear,false);
            }
            if(angular) {
                if(flags&1u)copy_vector(m,zero,angular);
                else native_campaign_object_motion_component_tu15(m,component,std::uint32_t(active_index),angular,true);
            }
            return;
        }
    }
    if(linear)copy_vector(m,record+0x88u,linear);
    if(angular)copy_vector(m,record+0x94u,angular);
}
}
