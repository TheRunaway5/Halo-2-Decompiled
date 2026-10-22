#include "halo2/object_sky_lighting.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
namespace {

void normalize_source(Vec3& v) {
    const long double x=v.x,y=v.y,z=v.z;
    const auto length=std::sqrt((x*x+y*y)+z*z);
    if(std::abs(length)>=.0001f) {
        const float reciprocal=1.f/float(length);
        v={v.x*reciprocal,reciprocal*v.y,reciprocal*v.z};
    }
}
}
bool Cache::object_sky_lighting_available(std::size_t ordinal) const {
    const auto scenario=query_tag({.group=tag_groups::scenario, .name=scenario_});
    if(!scenario)return false;
    const Bytes data(cache_bytes());
    const auto root=metadata_offset(scenario->address,scenario->size);
    const auto skies=TagAddressSpace(data, metadata_magic_).block(root+8, 8);
    if(ordinal>=skies.count)return false;
    const auto datum=data.u32(skies.offset+ordinal*8+4);
    if(datum==0xffffffffu)return false;
    const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::sky, .require_address=true});
    if(!tag)return false;
    if(tag->size<0x14)throw FormatError("truncated render sky lighting availability");
    const auto base=metadata_offset(tag->address,tag->size);
    return data.u32(base+4)!=0xffffffffu && (data.u32(base+0x10)&32);
}
std::optional<ObjectSkyLightingDefinition> Cache::load_object_sky_lighting(std::size_t ordinal) const {
    const auto scenario=query_tag({.group=tag_groups::scenario, .name=scenario_});
    if(!scenario)return {};
    const Bytes data(cache_bytes());
    const auto root=metadata_offset(scenario->address,scenario->size);
    const auto skies=TagAddressSpace(data, metadata_magic_).block(root+8, 8);
    if(ordinal>=skies.count)throw FormatError("object lighting sky outside scenario");
    const auto datum=data.u32(skies.offset+ordinal*8+4);
    if(datum==0xffffffffu)return {};
    const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::sky});
    if(!tag || tag->size<0xac)throw FormatError("missing/truncated object lighting sky");
    const auto base=metadata_offset(tag->address,tag->size);
    ObjectSkyLightingDefinition out;
    out.name=tag->name;out.datum=datum;
    out.outdoor_ambient={data.f32(base+0x34),data.f32(base+0x38),data.f32(base+0x3c)};
    const auto lights=TagAddressSpace(data, metadata_magic_).block(base+0x78, 0x34);
    for(std::size_t i=0;i<lights.count;++i) {
        const auto p=lights.offset+i*0x34;
        ObjectSkyLightingDefinition::Light light;
        light.yaw=data.f32(p+0xc);light.pitch=data.f32(p+0x10);
        const auto radiosity=TagAddressSpace(data, metadata_magic_).block(p+0x2c, 0x28);
        if(radiosity.count) {
            const auto r=radiosity.offset;
            light.radiosity={{data.f32(r+4),data.f32(r+8),data.f32(r+12)},data.f32(r+16)};
        }
        out.lights.push_back(light);
    }
    return out;
}
ObjectSkyLightFrame object_sky_light_frame(const ObjectSkyLightingDefinition& definition) {
    ObjectSkyLightFrame frame;
    frame.outdoor_ambient=definition.outdoor_ambient;
    frame.test_collision=!definition.lights.empty();
    float accumulated_power=0;
    for(std::size_t i=0;i<definition.lights.size();++i) {
        const auto& light=definition.lights[i];
        if(!light.radiosity)continue;
        // D1B1C uses x87 trig/products. X's product remains extended until
        // the accumulated sum; Y/Z are rounded before their additions.
        const long double pitch=light.pitch,yaw=light.yaw;
        const auto cosine_pitch=std::cos(pitch);
        frame.incident.x=float(std::cos(yaw)*cosine_pitch+frame.incident.x);
        frame.incident.y=float(std::sin(yaw)*cosine_pitch)+frame.incident.y;
        frame.incident.z=float(std::sin(pitch))+frame.incident.z;
        const auto& radiosity=*light.radiosity;
        if(i==0) {frame.radiosity=radiosity.color;accumulated_power=radiosity.power;}
        else if(radiosity.power>0) {
            const float ratio=accumulated_power/radiosity.power;
            const float weight=std::clamp(1.f-ratio/(float(i)+2.f),0.f,1.f);
            const float previous_weight=1.f-weight;
            frame.radiosity={frame.radiosity.x*previous_weight+radiosity.color.x*weight,
                frame.radiosity.y*previous_weight+radiosity.color.y*weight,
                frame.radiosity.z*previous_weight+radiosity.color.z*weight};
            accumulated_power=accumulated_power+radiosity.power;
        }
    }
    if(!frame.test_collision)frame.incident={-1,-1,-1};
    normalize_source(frame.incident);
    return frame;
}
ObjectLightmapSample object_sky_light_sample(const ObjectSkyLightFrame& frame,
    std::optional<std::int16_t> material) {
    const auto color=frame.test_collision && material && *material==-1?frame.radiosity:frame.outdoor_ambient;
    const Vec3 clamped{std::clamp(color.x,0.f,1.f),std::clamp(color.y,0.f,1.f),std::clamp(color.z,0.f,1.f)};
    return {frame.incident,0xff555555u,pack_object_lighting_rgb(clamped)};
}
} // namespace halo2
