#include "halo2/source_lens_query_geometry_tu15.hpp"
#include "halo2/cache.hpp"
#include <cstring>

namespace halo2 {
SourceLensQueryDraw source_lens_query_geometry_tu15(std::span<const std::byte> packet,
    std::uint32_t sample_area,std::uint32_t id,std::uint64_t generation) {
    SourceLensQueryDraw draw;draw.id=id;draw.generation=generation;
    if(!sample_area && packet.empty())return draw;
    if(!sample_area || packet.size()!=96)
        throw FormatError("Original lens query geometry lacks its complete callback packet");
    const Bytes words(packet);
    if(words.u32(0)!=0x417fcu || words.u32(4)!=7 ||
       words.u32(88)!=0x417fcu || words.u32(92)!=0)
        throw FormatError("Original lens query begin/end packet differs");
    for(unsigned n=0;n<4;++n) {
        if(words.u32(8+n*20)!=0x101a00u)
            throw FormatError("Original lens query vertex method differs");
        std::memcpy(draw.vertices[n].data(),packet.data()+12+n*20,16);
    }
    draw.has_quad=true;return draw;
}
}
