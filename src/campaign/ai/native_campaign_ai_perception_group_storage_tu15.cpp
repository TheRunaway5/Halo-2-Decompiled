#include "halo2/native_campaign_ai_perception_group_storage_tu15.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t at,std::uint32_t value,unsigned width=4) {
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
    m.write(at,std::span(bytes).first(width));
}
std::int16_t signed16(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
std::uint32_t row(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride){return rows+(datum&0xffffu)*stride;}
}
std::uint32_t native_campaign_ai_perception_group_find_tu15(
        const SourcePhysicsContactMemory &m,std::uint32_t clump,std::uint32_t object) {
    const auto objects=read(m,0x4ecbd0u),objects_rows=read(m,objects+0x44u);
    const auto body=read(m,row(objects_rows,object,12u)+8u);
    const auto kind=read(m,body+0xaau,1);
    auto attached_owner=none;
    if(kind==12u && read(m,body+0x134u)==0u) {
        const auto extension=body+std::uint32_t(std::int32_t(signed16(read(m,body+0x13au,2))));
        if(extension)attached_owner=read(m,extension+4u);
    }
    const auto groups=read(m,0x50eef4u),clumps=read(m,0x50eef8u),clump_rows=read(m,clumps+0x44u);
    auto member=read(m,row(clump_rows,clump,0x50u)+0x14u);
    while(member!=none) {
        const auto rows=read(m,groups+0x44u),record=row(rows,member,0xc4u);
        const auto target=read(m,record+8u),current=member;
        member=read(m,record+0x14u);
        if(target==object)return current;
        if(read(m,record+0x22u,1) && attached_owner!=none && attached_owner==read(m,record+0x1cu))return current;
    }
    return none;
}
bool native_campaign_ai_perception_group_active_tu15(const SourcePhysicsContactMemory &m,std::uint32_t group) {
    const auto groups=read(m,0x50eef4u),rows=read(m,groups+0x44u),record=row(rows,group,0xc4u);
    if(signed16(read(m,record+4u,2))>=1)return true;
    auto property=read(m,record+0x18u);
    const auto properties=read(m,0x50eef0u);
    if(property==none)return false;
    while(true) {
        const auto property_rows=read(m,properties+0x44u),member=row(property_rows,property,0x3cu);
        const auto state=signed16(read(m,member+0x24u,2));
        property=read(m,member+0x34u);
        if(state>=1)return true;
        if(property==none)return false;
    }
}
void native_campaign_ai_perception_point_reset_tu15(SourcePhysicsContactMemory &m,std::uint32_t point) {
    const auto default_point=read(m,0x474ce8u);
    write(m,point+0x40u,none);
    for(const auto offset:{0x66u,0x62u,0x5eu,0x64u,0x61u,0x5fu})write(m,point+offset,0,1);
    write(m,point,none);write(m,point+0x60u,1,1);
    for(const auto offset:{0x63u,0x69u,0x65u})write(m,point+offset,0,1);
    write(m,point+0x3cu,none);write(m,point+0x67u,0,1);
    for(unsigned lane=0;lane<3;++lane)write(m,point+0x48u+lane*4u,read(m,default_point+lane*4u));
    write(m,point+0x54u,none,2);write(m,point+0x44u,none);write(m,point+0x58u,0,1);
}
std::uint32_t native_campaign_ai_perception_property_create_tu15(SourcePhysicsContactMemory &m,std::uint32_t group) {
    const auto groups=read(m,0x50eef4u),group_rows=read(m,groups+0x44u),record=row(group_rows,group,0xc4u);
    const auto properties=read(m,0x50eef0u);
    const auto property=native_tu15_data_array_allocate(m,properties);
    if(property==none)return none;
    const auto rows=read(m,properties+0x44u),member=row(rows,property,0x3cu);
    write(m,member+0x10u,0);
    const auto maximum_distance=read(m,0x45695cu);
    write(m,member+0xcu,0,2);write(m,member+0x14u,none);write(m,member+0x1cu,none,2);
    write(m,member+0x20u,read(m,record+8u));
    write(m,member+0x18u,read(m,record+2u,2),2);
    write(m,member+0x24u,read(m,record+4u,2),2);
    write(m,member+8u,none);write(m,member+4u,none);write(m,member+0x28u,maximum_distance);
    write(m,member+0x27u,0,1);
    for(const auto offset:{0x2cu,0x30u,0x34u,0x38u})write(m,member+offset,none);
    return property;
}
}
