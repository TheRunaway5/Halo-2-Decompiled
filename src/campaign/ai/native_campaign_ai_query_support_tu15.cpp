#include "halo2/native_campaign_ai_query_support_tu15.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value,unsigned width=4) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
    memory.write(at,std::span(bytes).first(width));
}
std::int16_t signed16(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
}
std::uint32_t native_campaign_ai_query_create_tu15(SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto actors=read(memory,0x5020c8u);
    const auto actor_rows=read(memory,actors+0x44u);
    if(read(memory,actor_rows+(actor&0xffffu)*0x888u+0x7cu)==0xffffffffu)return 0xffffffffu;
    const auto queries=read(memory,0x50eefcu);
    const auto datum=native_tu15_data_array_allocate(memory,queries);
    if(datum==0xffffffffu)return datum;
    const auto query_rows=read(memory,queries+0x44u);
    const auto query=query_rows+(datum&0xffffu)*0xbcu;
    for(unsigned n=0;n<9;++n) {
        write(memory,query+0x10u+n*12u,0xffffffffu);
        write(memory,query+0x14u+n*12u,0,2);
    }
    write(memory,query+4u,actor);
    write(memory,query+8u,3,2);
    write(memory,query+0x7cu,1,2);
    return datum;
}
bool native_campaign_ai_query_attach_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t actor,std::int16_t slot,std::uint32_t behavior) {
    const auto actors=read(memory,0x5020c8u);
    const auto queries=read(memory,0x50eefcu);
    const auto actor_rows=read(memory,actors+0x44u);
    const auto actor_record=actor_rows+(actor&0xffffu)*0x888u;
    const auto query_rows=read(memory,queries+0x44u);
    const auto reservation=actor_record+0x194u+std::uint32_t(std::int32_t(slot))*12u;
    const auto member=read(memory,reservation+2u,2);
    const auto datum=read(memory,reservation+4u);
    const auto query=query_rows+(datum&0xffffu)*0xbcu;
    write(memory,query+8u+std::uint32_t(std::int32_t(signed16(member)))*12u,3,2);
    write(memory,behavior+0x18u,member,2);
    write(memory,behavior+0xcu,0,1);
    write(memory,behavior+0x10u,read(memory,reservation+4u));
    write(memory,query+0x7cu,std::uint16_t(read(memory,query+0x7cu,2)+1u),2);
    write(memory,reservation,0xffffu,2);
    return true;
}
std::uint32_t native_campaign_ai_query_next_tu15(SourcePhysicsContactMemory &memory,std::uint32_t iterator) {
    const auto previous=signed16(read(memory,iterator+4u,2));
    if(previous>=4)return 0;
    write(memory,iterator+4u,previous==-1?0:std::uint16_t(std::uint16_t(previous)+1u),2);
    const auto actors=read(memory,0x5020c8u);
    const auto actor_rows=read(memory,actors+0x44u);
    const auto actor=read(memory,iterator);
    const auto actor_record=actor_rows+(actor&0xffffu)*0x888u;
    if(signed16(read(memory,iterator+4u,2))>=4)return 0;
    const auto clock=read(memory,0x51df54u);
    const auto kind=read(memory,iterator+6u,2);
    while(true) {
        auto index=std::uint16_t(read(memory,iterator+4u,2));
        const auto slot=actor_record+0x194u+std::uint32_t(std::int32_t(signed16(index)))*12u;
        if(read(memory,slot,2)==kind) {
            const auto expiry=read(memory,slot+8u);
            if(signed32(expiry)>signed32(read(memory,clock+8u)))return slot;
        }
        index=std::uint16_t(index+1u);
        write(memory,iterator+4u,index,2);
        if(signed16(index)>=4)return 0;
    }
}
}
