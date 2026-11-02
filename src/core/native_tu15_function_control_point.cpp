#include "halo2/native_tu15_function_control_point.hpp"
#include <array>
namespace halo2 {
std::uint32_t native_tu15_authored_function_control_point(SourcePhysicsContactMemory&memory,
    std::uint32_t reference,std::uint32_t layer,std::uint32_t index){
    const auto read=[&](std::uint32_t p,unsigned width){std::array<std::byte,4>b{};
        memory.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;};
    const auto blob=read(reference+4u,4),kind=read(blob,1);
    if(kind<4u||(kind>8u&&kind!=10u))return 0;
    const auto current_kind=read(blob,1),stride=read(0x44b884u+current_kind,1);
    const auto offset=(stride*(layer&0xffu)+(index&0xffu)*2u)*4u;
    return blob+offset+0x14u;
}
} // namespace halo2
