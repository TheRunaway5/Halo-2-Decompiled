#include "halo2/native_campaign_ai_perception_lookup_tu15.hpp"
#include "halo2/native_campaign_ai_object_actor_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_link_tu15.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
    memory.write(at,bytes);
}
}
std::uint32_t native_campaign_ai_perception_lookup_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t object,std::uint32_t actor,bool create,NativeTu15PhysicsFrameFp &fp,
    unsigned occupied_x87) {
    if(object==none)return none;
    const auto actors=read(memory,0x5020c8u);
    const auto actor_rows=read(memory,actors+0x44u);
    const auto actor_record=actor_rows+(actor&0xffffu)*0x888u;
    const auto target_actor=native_campaign_ai_object_actor_tu15(memory,object);
    if(target_actor==actor)return none;
    auto property=read(memory,actor_record+0x58u);
    const auto groups=read(memory,0x50eef4u);
    const auto properties=read(memory,0x50eef0u);
    while(property!=none) {
        const auto rows=read(memory,properties+0x44u);
        const auto record=rows+(property&0xffffu)*0x3cu;
        const auto group_datum=read(memory,record+8u);
        const auto group_rows=read(memory,groups+0x44u);
        const auto group=group_rows+(group_datum&0xffffu)*0xc4u;
        const auto same_object=read(memory,record+0x20u)==object;
        const auto next=read(memory,record+0x2cu);
        if(same_object)return property;
        if(read(memory,group+0x22u,1)) {
            const auto owner=read(memory,group+0x1cu);
            if(owner!=none && owner==target_actor)return property;
        }
        property=next;
    }
    if(!create || !read(memory,actor_record+9u,1) ||
        read(memory,actor_record+0x7cu)==none)return none;
    const auto created=native_campaign_ai_perception_group_link_tu15(memory,object,actor,3u,fp,occupied_x87);
    if(created==none)return none;
    const auto clock=read(memory,0x51df54u);
    const auto rate=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(memory,clock+2u,2))));
    const auto now=read(memory,clock+8u);
    const auto current_properties=read(memory,0x50eef0u);
    const auto current_property_rows=read(memory,current_properties+0x44u);
    const auto group_datum=read(memory,current_property_rows+(created&0xffffu)*0x3cu+8u);
    const auto current_groups=read(memory,0x50eef4u);
    const auto current_group_rows=read(memory,current_groups+0x44u);
    write(memory,current_group_rows+(group_datum&0xffffu)*0xc4u+0x10u,
        now+std::uint32_t(rate*60));
    return created;
}
}
