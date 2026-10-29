#include "halo2/native_campaign_ai_perception_state_tu15.hpp"
#include <array>
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
}
bool native_campaign_ai_perception_available_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t perception) {
    const auto group_datum=read(memory,perception+8u);
    const auto groups=read(memory,0x50eef4u);
    const auto group_rows=read(memory,groups+0x44u);
    const auto tracking=read(memory,perception+0x14u);
    const auto group=group_rows+(group_datum&0xffffu)*0xc4u;
    if(tracking!=0xffffffffu) {
        const auto actor=read(memory,group+0x1cu);
        if(actor==0xffffffffu)return true;
        const auto actors=read(memory,0x5020c8u);
        const auto actor_rows=read(memory,actors+0x44u);
        const auto record=actor_rows+(actor&0xffffu)*0x888u;
        if(read(memory,record+4u,2)==0x0fu)return false;
        if(!read(memory,record+0x267u,1))return true;
        return read(memory,record+0x268u,1)!=0;
    }
    if(!read(memory,group+0x25u,1))return false;
    return read(memory,group+0x23u,1)==0;
}
void native_campaign_ai_perception_reset_tu15(
    SourcePhysicsContactMemory &memory,std::uint32_t datum) {
    const auto data=read(memory,0x50eef0u);
    const auto records=read(memory,data+0x44u);
    const auto record=records+(datum&0xffffu)*0x3cu;
    const auto tracking=read(memory,record+0x14u);
    if(tracking!=0xffffffffu) {
        const auto tracked=read(memory,0x50eeecu);
        const auto tracked_record=read(memory,tracked+0x44u)+(tracking&0xffffu)*0x124u;
        const auto extension=tracked_record+0x70u;
        if(tracked_record && extension) {
            write(memory,extension+0x68u,0,1);
            write(memory,extension+0x69u,0,1);
            write(memory,extension+0x4cu,0,1);
            write(memory,extension+0x70u,0,2);
            write(memory,extension+0x6cu,1,1);
            write(memory,extension+0x6du,1,1);
            write(memory,extension+0x88u,0,1);
            write(memory,extension+0x90u,0,2);
            write(memory,extension+0x8au,0,2);
        }
    }
    // Earlier extension stores may publish a different datum here. Resolve
    // the actual current group only after those stores, using the held row.
    const auto group_datum=read(memory,record+8u);
    const auto groups=read(memory,0x50eef4u);
    const auto group_rows=read(memory,groups+0x44u);
    const auto group=group_rows+(group_datum&0xffffu)*0xc4u;
    write(memory,group+0x32u,0,1);
    write(memory,group+0x33u,0,1);
    write(memory,group+0x34u,0,1);
}
}
