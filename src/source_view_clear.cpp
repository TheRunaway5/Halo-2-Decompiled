#include "halo2/source_view_clear.hpp"
#include "halo2/source_fog_clear.hpp"
#include <bit>

namespace halo2 {
std::vector<SourceClearEvent> source_clear_plan(std::uint32_t semantic,
    std::uint32_t argb,std::uint32_t depth,std::uint8_t stencil,
    std::uint8_t temporary,std::int32_t target,const SourceClearViewport &viewport) {
    const auto hardware=((semantic&1u)?0xf0u:0u)|((semantic&0x1eu)<<3)|((semantic>>5)&3u);
    if(!hardware)return {};
    std::vector<SourceClearEvent> result;
    if(temporary && target==0) {
        result.push_back({SourceClearEvent::Kind::set_viewport,{0,0,640,480,0,0x3f800000}});
        result.push_back({SourceClearEvent::Kind::clear,{0xf0,0,0,0,0,0}});
        result.push_back({SourceClearEvent::Kind::set_viewport,viewport});
    }
    result.push_back({SourceClearEvent::Kind::clear,{hardware,argb,depth,stencil,0,0}});
    return result;
}
std::vector<SourceClearEvent> source_view_initialization_plan(
    std::span<const std::byte> header,std::uint8_t transition,
    std::uint16_t control,const SourceClearViewport &viewport) {
    if(header.size()!=0x298)throw FormatError("Source view clear requires the complete original header");
    const auto byte=[&](std::size_t i){return std::to_integer<std::uint32_t>(header[i]);};
    const auto half=[&](std::size_t i){return byte(i)|(byte(i+1)<<8);};
    const auto word=[&](std::size_t i){return half(i)|(half(i+2)<<16);};
    const auto target=half(2)==1?18u:0u;
    std::vector<SourceClearEvent> result{{SourceClearEvent::Kind::bind_target,{target,1,0,0,0,0}}};
    if(!half(8))return result;
    const auto rgb=transition?0u:source_clear_color_rgb8({std::bit_cast<float>(word(12)),
        std::bit_cast<float>(word(16)),std::bit_cast<float>(word(20))},control);
    auto clears=source_clear_plan(transition?1u:0x61u,rgb,0x3f800000,0,
        std::uint8_t(byte(7)),std::int32_t(target),viewport);
    result.insert(result.end(),clears.begin(),clears.end());
    return result;
}
}
