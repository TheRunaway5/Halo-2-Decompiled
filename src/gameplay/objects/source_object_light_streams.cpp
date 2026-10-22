#include "halo2/source_object_light_streams.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/bsp_surface_lighting.hpp"

namespace halo2 {
namespace {
std::array<std::uint16_t,3> triangle_indices(const SourceObjectLightDataGeometry& geometry,
    std::int32_t triangle) {
    if(triangle<0)throw FormatError("object lighting triangle precedes resident index bank");
    const auto at=std::uint64_t(std::uint32_t(triangle))*3;
    if(at+2>=geometry.indices.size())throw FormatError("object lighting triangle escapes resident index bank");
    return {geometry.indices[std::size_t(at)],geometry.indices[std::size_t(at+1)],
        geometry.indices[std::size_t(at+2)]};
}
Vec3 argb_color(std::uint32_t color) {
    constexpr float inverse=0.003921568859368563f; //literal45DBB4/131BB0.
    return {float((color>>16)&255)*inverse,float((color>>8)&255)*inverse,
        float(color&255)*inverse};
}
float blend(float a,float b,float c,float u,float v) {
    return (c-a)*v+(b-a)*u+a;
}
}
std::optional<Vec3> source_object_light_radiance(const SourceObjectLightDataGeometry& geometry,
    std::int32_t triangle,float u,float v) {
    if(geometry.streams.size()<=3)return {};
    std::size_t selected=3;bool rgb24=false;
    for(;selected<geometry.streams.size();++selected) {
        const auto declaration=geometry.streams[selected].declaration;
        if(declaration==46)break;
        if(declaration==47){rgb24=true;break;}
    }
    if(selected==geometry.streams.size())--selected;
    const auto indices=triangle_indices(geometry,triangle);
    const Bytes bytes(geometry.streams[selected].data);std::array<Vec3,3> colors;
    for(unsigned i=0;i<3;++i) {
        const auto at=std::size_t(indices[i])*(rgb24?3:4);
        if(rgb24) {
            bytes.require(at,3);
            colors[i]=decode_bsp_radiance({std::to_integer<std::uint8_t>(bytes.slice(at,1)[0]),
                std::to_integer<std::uint8_t>(bytes.slice(at+1,1)[0]),
                std::to_integer<std::uint8_t>(bytes.slice(at+2,1)[0])});
        } else colors[i]=argb_color(bytes.u32(at));
    }
    return Vec3{blend(colors[0].x,colors[1].x,colors[2].x,u,v),
        blend(colors[0].y,colors[1].y,colors[2].y,u,v),
        blend(colors[0].z,colors[1].z,colors[2].z,u,v)};
}
std::optional<Vec3> source_object_light_incident(const SourceObjectLightDataGeometry& geometry,
    std::int32_t triangle,float u,float v) {
    if(geometry.streams.size()<=3)return {};
    std::size_t selected=3;
    for(;selected<geometry.streams.size();++selected)
        if(geometry.streams[selected].declaration==48)break;
    if(selected==geometry.streams.size())--selected;
    const auto indices=triangle_indices(geometry,triangle);
    const Bytes bytes(geometry.streams[selected].data);std::array<Vec3,3> incident;
    for(unsigned i=0;i<3;++i)
        incident[i]=decode_cpu_lightmap_incident(bytes.u32(std::size_t(indices[i])*4));
    //D31C0's X expression starts with B; Y/Z start with C. Each operation
    //rounds separately and no directional-strength normalization follows.
    return Vec3{(incident[1].x-incident[0].x)*u+(incident[2].x-incident[0].x)*v+incident[0].x,
        blend(incident[0].y,incident[1].y,incident[2].y,u,v),
        blend(incident[0].z,incident[1].z,incident[2].z,u,v)};
}
} // namespace halo2
