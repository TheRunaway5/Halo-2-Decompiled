#include "halo2/native_campaign_ai_perception_refs_tu15.hpp"
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
std::uint32_t signed_kind(std::uint32_t value) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
}
}
std::uint32_t native_campaign_ai_perception_position_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t perception) {
    const auto kind=signed_kind(read(memory,perception+0x18u,2));
    if(read(memory,0x47d636u+kind*16u,2)==1u) {
        const auto datum=read(memory,perception+8u);
        const auto pool=read(memory,0x50eef4u);
        const auto records=read(memory,pool+0x44u);
        return records+(datum&0xffffu)*0xc4u+0x58u;
    }
    const auto tracking=read(memory,perception+0x14u);
    if(tracking!=0xffffffffu) {
        const auto pool=read(memory,0x50eeecu);
        const auto records=read(memory,pool+0x44u);
        return records+(tracking&0xffffu)*0x124u+4u;
    }
    const auto datum=read(memory,perception+8u);
    const auto pool=read(memory,0x50eef4u);
    const auto records=read(memory,pool+0x44u);
    return records+(datum&0xffffu)*0xc4u+0x58u;
}
std::uint32_t native_campaign_ai_perception_tracking_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t perception) {
    const auto datum=read(memory,perception+0x14u);
    if(datum==0xffffffffu)return 0;
    const auto pool=read(memory,0x50eeecu);
    const auto record=read(memory,pool+0x44u)+(datum&0xffffu)*0x124u;
    return record?record+0x70u:0;
}
std::uint32_t native_campaign_ai_perception_datum_tracking_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t datum) {
    const auto data=read(memory,0x50eef0u);
    const auto datum_record=read(memory,data+0x44u)+(datum&0xffffu)*0x3cu;
    const auto tracking=read(memory,datum_record+0x14u);
    if(tracking==0xffffffffu)return 0;
    const auto pool=read(memory,0x50eeecu);
    const auto record=read(memory,pool+0x44u)+(tracking&0xffffu)*0x124u;
    return record?record+0x70u:0;
}
}
