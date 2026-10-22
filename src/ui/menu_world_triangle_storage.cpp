#include "halo2/menu_world_triangle_storage.hpp"
#include <algorithm>
#include <limits>

namespace halo2 {
std::vector<std::uint32_t> menu_world_triangle_storage(const MenuWorldSourceStoredDraw &stored) {
    const auto &draw=stored.source.draw();
    const auto &raw=stored.raw_storage;
    if(!draw.geometry||!raw.buffer||raw.offset%4||!raw.size||raw.size%4||raw.size>UINT32_MAX||
       stored.topology!=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST||stored.index_type!=VK_INDEX_TYPE_UINT16||
       draw.triangle_count()>UINT32_MAX/3||stored.index_count!=draw.triangle_count()*3||!stored.index_count||
       stored.resources.size()!=draw.geometry->resources().size())
        throw FormatError("world triangle vertex storage requires a bounded padded raw bank and ordered source triangles");
    const std::uint64_t metadata_bytes=(64ull+stored.index_count)*4;
    if(metadata_bytes>std::numeric_limits<std::size_t>::max())
        throw FormatError("world triangle metadata exceeds host allocation range");
    std::uint64_t resource_end=0;
    for(unsigned i=0;i<stored.resources.size();++i) {
        const auto &resource=stored.resources[i];
        const auto expected_offset=(resource_end+3)&~std::uint64_t{3};
        if(resource.buffer!=raw.buffer||resource.offset<raw.offset||
           resource.offset-raw.offset!=expected_offset||
           resource.size!=draw.geometry->resources()[i].bytes.size()||
           expected_offset>raw.size||resource.size>raw.size-expected_offset)
            throw FormatError("world triangle resource bank differs from its immutable source layout");
        resource_end=expected_offset+resource.size;
    }
    if(((resource_end+3)&~std::uint64_t{3})!=raw.size)
        throw FormatError("world triangle raw bank has a different padded extent");
    std::vector<std::uint32_t> words(64+stored.index_count);
    unsigned maximum_vertex=0;
    for(std::size_t primitive=0;primitive<draw.triangle_count();++primitive) {
        const auto triangle=draw.triangle(primitive);
        for(unsigned lane=0;lane<3;++lane) {
            const auto vertex=triangle[lane];
            if(vertex>=draw.geometry->vertex_count())throw FormatError("world triangle index exceeds actual source vertices");
            words[64+primitive*3+lane]=vertex;maximum_vertex=std::max(maximum_vertex,unsigned(vertex));
        }
    }
    std::array<bool,16> occupied{};
    for(const auto &input:stored.source.inputs()) {
        const auto &stream=input.stream;
        if(input.vertex_register>=16||occupied[input.vertex_register]||stream.resource>=stored.resources.size()||!stream.byte_stride)
            throw FormatError("world triangle stream register/resource/stride is invalid");
        occupied[input.vertex_register]=true;
        unsigned width{},code{};
        switch(stream.format) {
        case MenuWorldSourceFormat::float32x3:width=12;code=0;break;
        case MenuWorldSourceFormat::float32x2:width=8;code=1;break;
        case MenuWorldSourceFormat::snorm16x3:width=6;code=2;break;
        case MenuWorldSourceFormat::snorm16x2:width=4;code=3;break;
        case MenuWorldSourceFormat::packed_signed_11_11_10:width=4;code=4;break;
        case MenuWorldSourceFormat::unorm8x1:width=1;code=5;break;
        case MenuWorldSourceFormat::unorm8x2:width=2;code=6;break;
        case MenuWorldSourceFormat::unorm8x3:width=3;code=7;break;
        case MenuWorldSourceFormat::unorm8x4:width=4;code=8;break;
        default:throw FormatError("world triangle stream has no admitted raw format");
        }
        const auto &resource=stored.resources[stream.resource];
        const std::uint64_t last=std::uint64_t(maximum_vertex)*stream.byte_stride+stream.byte_offset+width;
        if(resource.buffer!=raw.buffer||resource.offset<raw.offset||resource.offset-raw.offset>raw.size||
           resource.size!=draw.geometry->resources()[stream.resource].bytes.size()||
           resource.size>raw.size-(resource.offset-raw.offset)||last>resource.size)
            throw FormatError("world triangle fetch exceeds its actual source resource/raw storage");
        const auto address=resource.offset-raw.offset+stream.byte_offset;
        if(address>UINT32_MAX||last+resource.offset-raw.offset>UINT32_MAX)
            throw FormatError("world triangle shader byte address overflows uint32");
        const auto at=input.vertex_register*4;
        words[at]=std::uint32_t(address);words[at+1]=stream.byte_stride;words[at+2]=code;
    }
    for(auto consumed:stored.source.consumed_registers())
        if(consumed>=16||(!occupied[consumed]&&std::ranges::find(stored.source.external_attribute_registers(),consumed)==stored.source.external_attribute_registers().end()))
            throw FormatError("world triangle draw consumes an unbound raw register");
    return words;
}
}
