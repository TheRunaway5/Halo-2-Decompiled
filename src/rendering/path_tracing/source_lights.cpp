#include "halo2/path_tracing/source_lights.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/object_sky_lighting.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2::path_tracing {
namespace {
bool finite(Vec3 v) {return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);}
void append(std::vector<Light>& out,Vec3 position,float radius,const EffectLightState& state) {
    if(!finite(position) || !finite(state.diffuse) || !std::isfinite(radius) ||
       !std::isfinite(state.brightness))throw FormatError("Path Tracing source light has nonfinite state");
    if(!(radius>0) || !(state.brightness>0) ||
       !(state.diffuse.x>0 || state.diffuse.y>0 || state.diffuse.z>0))return;
    out.push_back({position,radius,state.diffuse,state.brightness});
}
}
std::vector<Light> source_lights(PhysicsLiveWorldTu15& world,
    std::span<const SourceEffectLightViewRecord> records) {
    std::vector<Light> out;out.reserve(records.size());
    for(const auto& row:records) {
        const auto& runtime=row.runtime;
        // Record+100 is a camera fade. The occupied/active bit and current
        // diffuse/brightness state establish emitter lifetime for RT instead.
        if(!row.definition || runtime.identity==0xffffffffu || !(runtime.flags&2) ||
           !(runtime.state.brightness>0) ||
           !(runtime.state.diffuse.x>0 || runtime.state.diffuse.y>0 || runtime.state.diffuse.z>0))continue;
        const auto shape=pstencil::native_light_shape_tu15(world,runtime.identity);
        const auto value=[&](unsigned index) {
            for(unsigned byte=0;byte<4;++byte)if(!shape.written[index*4+byte])
                throw FormatError("Path Tracing native light consumed an unwritten shape field");
            return std::bit_cast<float>(shape.words.at(index));
        };
        const Vec3 origin{value(11),value(12),value(13)};
        if(shape.type()==0) {
            append(out,origin,value(15),runtime.state);
            continue;
        }
        const auto cameras=pstencil::native_light_cameras_tu15(world,runtime.identity);
        if(cameras.size()!=1 || !cameras.front().full_frustum_enabled)
            throw FormatError("Path Tracing projected light has no completed source frustum");
        const auto& frustum=cameras.front().full_frustum;
        float squared_radius{};
        for(const auto corner:frustum.corners) {
            if(!finite(corner))throw FormatError("Path Tracing projected light has a nonfinite source corner");
            const auto x=corner.x-origin.x,y=corner.y-origin.y,z=corner.z-origin.z;
            squared_radius=std::max(squared_radius,(x*x+y*y)+z*z);
        }
        const auto count=out.size();
        // This enclosing radius is only the optional RT radiometric bound;
        // all six original planes retain the actual projected light shape.
        append(out,origin,std::sqrt(squared_radius),runtime.state);
        if(out.size()==count)continue;
        for(const auto& plane:frustum.planes)for(const auto component:plane)
            if(!std::isfinite(component))throw FormatError("Path Tracing projected light has a nonfinite source plane");
        out.back().planes=frustum.planes;
    }
    return out;
}
std::vector<Light> source_lights(std::span<const WorldEffectLight> records) {
    std::vector<Light> out;out.reserve(records.size());
    for(const auto& light:records)append(out,light.position,light.radius*light.state.size,light.state);
    return out;
}
std::optional<Light> source_sky_light(const ObjectSkyLightingDefinition& definition) {
    const auto frame=object_sky_light_frame(definition);
    if(!frame.test_collision)return {};
    if(!finite(frame.incident) || !finite(frame.radiosity))
        throw FormatError("Path Tracing source sky has nonfinite incident/radiosity");
    const auto& d=frame.incident;
    if(!((d.x*d.x+d.y*d.y)+d.z*d.z>1.e-18f) ||
       !(frame.radiosity.x>0 || frame.radiosity.y>0 || frame.radiosity.z>0))return {};
    Light light;light.color=frame.radiosity;light.direction=frame.incident;return light;
}
}
