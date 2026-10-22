#include "halo2/source_object_light_uv.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
float midpoint(std::int16_t v){return (float(v)*2.f+1.f)*std::bit_cast<float>(0x37800080u);}
float limit(float v){if(v<-1000.f)return -1000.f;if(v>1000.f)return 1000.f;return v;}
}
Vec2 source_object_light_uv(const SourceObjectLightDataGeometry& g,
    std::int32_t triangle,float u,float v) {
    if(g.streams.size()<2)throw FormatError("source object UV second descriptor missing");
    if(triangle<0)throw FormatError("source object UV triangle precedes index bank");
    const auto first=std::uint64_t(std::uint32_t(triangle))*3;
    if(first+2>=g.indices.size())throw FormatError("source object UV triangle escapes index bank");
    const auto& stream=g.streams[1];const Bytes bytes(stream.data);const bool floats=stream.declaration==24;
    std::array<Vec2,3> points;
    for(unsigned i=0;i<3;++i) {
        const auto offset=std::uint64_t(stream.descriptor_offset)+std::uint64_t(g.indices[std::size_t(first+i)])*(floats?8:4);
        if(offset>bytes.size())throw FormatError("source object UV offset escapes data bank");
        const auto at=std::size_t(offset);
        if(floats)points[i]={bytes.f32(at),bytes.f32(at+4)};
        else points[i]={midpoint(std::bit_cast<std::int16_t>(bytes.u16(at))),
            midpoint(std::bit_cast<std::int16_t>(bytes.u16(at+2)))};
    }
    const auto& a=points[0];const auto& b=points[1];const auto& c=points[2];
    const float x=((c.x-a.x)*v+(b.x-a.x)*u)+a.x;
    //The float2 Y branch adds B before C; SHORT2 uses C before B.
    const float y=floats?((b.y-a.y)*u+(c.y-a.y)*v)+a.y:
        ((c.y-a.y)*v+(b.y-a.y)*u)+a.y;
    return {limit(x),limit(y)};
}
}
