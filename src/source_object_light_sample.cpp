#include "halo2/source_object_light_sample.hpp"
#include <algorithm>
#include <bit>
namespace halo2 {
namespace {
std::array<SourcePrtContactPoint,3> prt_points(const SourceObjectLightDataGeometry& g,
    std::uint32_t strip,const SourceObjectLightPrtResource& resource) {
    if(std::uint64_t(strip)+2>=g.indices.size())throw FormatError("source object PRT strip escapes index bank");
    const Bytes bytes(resource.points);std::array<SourcePrtContactPoint,3> out;
    for(unsigned i=0;i<3;++i) {
        const auto at=std::size_t(g.indices[strip+i])*(resource.declaration==57?10:20);
        if(resource.declaration==57) {
            std::array<std::int16_t,5> encoded;
            for(unsigned k=0;k<5;++k)encoded[k]=std::bit_cast<std::int16_t>(bytes.u16(at+k*2));
            out[i]=source_prt_contact_short_point(encoded);
        } else {
            out[i].cluster=bytes.f32(at);for(unsigned k=0;k<4;++k)out[i].weights[k]=bytes.f32(at+4+k*4);
        }
    }return out;
}
float clamp(float v){return v<0?0:v>1?1:v;}
}
SourceObjectLightSampleStatus source_object_light_sample(const SourceObjectLightDataGeometry& g,
    const SourceObjectLightSampleContext& context,const SourceObjectLightDiffuseSample& diffuse,
    ObjectLightmapSample& output) {
    const auto& p=context.contact;
    if(p[2]==0xffffffffu)throw FormatError("source object-light sample requires type4 attribution");
    auto status=SourceObjectLightSampleStatus::miss;
    if(p[5]==0xffffffffu)return status; //D21AC:caller output remains untouched.
    const auto b=std::bit_cast<float>(p[12]),c=std::bit_cast<float>(p[13]);
    std::optional<SourcePrtContactSample> prt,baked;
    if(context.model_prt_count>0) {
        if(context.prt_resource)prt=source_prt_contact_sample(context.current_prt_cache,
            context.current_prt_count,context.current_prt_arena,p[2],p[7],std::bit_cast<std::int32_t>(p[11]),
            prt_points(g,p[9],*context.prt_resource),b,c);
        status=prt?SourceObjectLightSampleStatus::resolved:SourceObjectLightSampleStatus::unavailable;
    }
    const auto triangle=std::bit_cast<std::int32_t>(p[10]);
    if(const auto color=source_object_light_radiance(g,triangle,b,c))
        if(const auto incident=source_object_light_incident(g,triangle,b,c))baked=SourcePrtContactSample{*incident,*color};
    const auto mixed=source_object_light_mix(prt,baked,context.default_incident);
    output.incident=mixed.incident;output.lightmap_rgb=mixed.lightmap_rgb;
    const auto uv=source_object_light_uv(g,triangle,b,c);
    if(!diffuse)throw FormatError("source object material bitmap service missing");
    if(const auto rgb=diffuse(uv))output.diffuse_rgb=pack_object_lighting_rgb({clamp(rgb->x),clamp(rgb->y),clamp(rgb->z)});
    else output.diffuse_rgb=0xffc0c0d0u; //Object branch differs from map fallback.
    return status;
}
std::optional<Vec3> source_object_light_texture_diffuse(const Texture& texture,Vec2 uv) {
    const auto color=sample_object_texture(texture,uv,1.f);if(!color)return {};
    constexpr float inverse=std::bit_cast<float>(0x3b808081u);
    return Vec3{float((*color>>16)&255)*inverse,float((*color>>8)&255)*inverse,float(*color&255)*inverse};
}
}
