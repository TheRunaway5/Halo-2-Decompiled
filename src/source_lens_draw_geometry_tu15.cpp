#include "halo2/source_lens_draw_geometry_tu15.hpp"
#include <bit>
#include <cmath>
#include <cstring>
namespace halo2 {
SourceLensDraw source_lens_draw_geometry_tu15(std::span<const std::byte> packet,unsigned material,
    float secondary_alpha,bool depth_test) {
    if(packet.size()!=136 || !std::isfinite(secondary_alpha) || secondary_alpha<0 || secondary_alpha>1)
        throw FormatError("Original lens primitive lacks its complete geometry/attribute8 packet");
    const Bytes b(packet);
    if(b.u32(0)!=0x417fc || b.u32(4)!=7 || b.u32(8)!=0x41964 ||
        b.u32(128)!=0x417fc || b.u32(132)!=0)
        throw FormatError("Original lens primitive begin/color/end methods differ");
    SourceLensDraw draw;draw.material=material;draw.depth_test=depth_test;
    for(unsigned n=0;n<4;++n) {
        const auto at=16+n*28;
        if(b.u32(at)!=0x4190c || b.u32(at+8)!=0x101a00)
            throw FormatError("Original lens primitive UV/position methods differ");
        auto &v=draw.vertices[n];std::memcpy(v.position.data(),packet.data()+at+12,16);
        for(float x:v.position)if(!std::isfinite(x))throw FormatError("Original lens primitive position is not finite");
        if(v.position[3]<=0 || (n && v.position[3]!=draw.vertices[0].position[3]))
            throw FormatError("Original lens primitive lost its constant positive W plane");
        v.uv={float(std::bit_cast<std::int16_t>(b.u16(at+4))),float(std::bit_cast<std::int16_t>(b.u16(at+6)))};
        v.rgba=b.u32(12);v.secondary_alpha=secondary_alpha;
    }
    return draw;
}
}
