#include "halo2/native_campaign_ai_area_tu15.hpp"
#include "native_campaign_authored_area_iterator_tu15.hpp"
#include <array>

namespace halo2 { namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
}
bool native_campaign_ai_has_area_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto pool=read(memory,0x5020c8u),row=read(memory,pool+0x44u)+(actor&0xffffu)*0x888u;
    const bool mode=read(memory,row+0x26cu)!=0xffffffffu;
    const auto squad=std::uint16_t(read(memory,row+0x30u,2));
    detail::CampaignAuthoredAreaIteratorTu15 areas(memory,squad,mode);
    return areas.next()!=0xffffu;
}
std::uint16_t native_campaign_ai_area_behavior_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto pool=read(memory,0x5020c8u),row=read(memory,pool+0x44u)+(actor&0xffffu)*0x888u;
    const auto enabled=read(memory,row+0x225u,1);
    const auto fallback=read(memory,0x47c1a4u);
    return enabled && native_campaign_ai_has_area_tu15(memory,actor)?0x6bu:std::uint16_t(fallback);
}
}
