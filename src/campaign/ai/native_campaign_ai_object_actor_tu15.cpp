#include "halo2/native_campaign_ai_object_actor_tu15.hpp"
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
}
std::uint32_t native_campaign_ai_object_actor_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t object) {
    const auto pool=read(memory,0x4ecbd0u);
    const auto rows=read(memory,pool+0x44u);
    const auto record=read(memory,rows+(object&0xffffu)*12u+8u);
    const auto kind=read(memory,record+0xaau,1);
    if((1u<<(kind&31u))&3u)return read(memory,record+0x12cu);
    if(kind==12u && read(memory,record+0x134u)==0u) {
        const auto offset=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(memory,record+0x13au,2))));
        const auto extension=record+std::uint32_t(offset);
        if(extension)return read(memory,extension+4u);
    }
    return 0xffffffffu;
}
}
