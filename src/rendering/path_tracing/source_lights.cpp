#include "halo2/path_tracing/source_lights.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/object_sky_lighting.hpp"
#include "halo2/math.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_campaign_object_marker_request_tu15.hpp"
#include "halo2/native_campaign_object_motion_refs_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

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
void present(PhysicsLiveWorldTu15& world,Light& light,const SourceEffectLightRuntime& runtime,const SourceLightPoseProvider& poses) {
    if(!poses)return;
    std::uint32_t owner=runtime.owner;std::uint16_t node{};
    // +54 is an attachment SLOT, not a bone ordinal. BAC70/BACD0/BAD60
    // retain parent-marker ownership, selected permutations and fallback0.
    if(owner!=0xffffffffu && runtime.attached_node>=0) {
        (void)world.source_object_body(owner); // Validate its complete live salt.
        const auto name=world.render_attachment_marker(owner,runtime.attached_node);
        NativeTu15PhysicsFrameFp fp;
        const auto marker=native_campaign_object_marker_request_value_tu15(world.memory(),owner,false,name,fp);
        if(marker.count)owner=native_campaign_object_marker_owner_tu15(world.memory(),owner);
        node=marker.value.node;
    }else {
        // C2380's attached effect branch retains an explicit parent/node at
        // +58/+5C. Free effects retain NONE; emitter ownership is not attachment.
        const auto& memory=world.memory();
        const auto pool=Bytes(memory.view(0x4ecbdcu,4)).u32(0);
        const auto records=Bytes(memory.view(pool+0x44u,4)).u32(0);
        const Bytes record(memory.view(records+(runtime.identity&65535u)*272u,272));
        if(record.u16(0)!=std::uint16_t(runtime.identity>>16))
            throw FormatError("Path Tracing attached light changed its complete generation");
        owner=record.u32(0x58u);node=record.u16(0x5cu);
        if(owner==0xffffffffu || node==0xffffu || (runtime.flags&4u))return;
    }
    (void)world.source_object_body(owner);
    const auto shown=poses(owner,node);
    if(!shown)return;
    const auto body=world.source_object_body(owner);
    const Bytes fields(world.memory().view(body+0x114u,4));
    const auto extent=std::bit_cast<std::int16_t>(fields.u16(0));
    const auto offset=std::bit_cast<std::int16_t>(fields.u16(2));
    if(offset<0 || extent<0 || std::size_t(node+1u)*sizeof(Transform)>std::size_t(extent))
        throw FormatError("Path Tracing light marker escapes its native WORLD node bank");
    Transform source;
    static_assert(sizeof(Transform)==52);
    std::memcpy(&source,world.memory().view(body+unsigned(offset)+unsigned(node)*52u,52).data(),52);
    present_light_frame(light,source,*shown);
}
}
void present_light_frame(Light& light,const Transform& source,const Transform& shown) {
    if(std::memcmp(&source,&shown,sizeof(Transform))==0)return;
    if(!std::isfinite(source.scale) || !std::isfinite(shown.scale) || source.scale==0 || shown.scale==0)
        throw FormatError("Path Tracing attachment presentation has a degenerate frame");
    // Native shape construction stays authoritative. Only its frame changes,
    // by the same source-node to presented-node motion as its attached model.
    const auto motion=compose_transform(shown,inverse_transform(source));
    const auto inverse=inverse_transform(motion);
    light.position=motion.position+(motion.basis[0]*light.position.x+
        motion.basis[1]*light.position.y+motion.basis[2]*light.position.z)*motion.scale;
    light.radius*=std::abs(motion.scale);
    for(auto& plane:light.planes) {
        const Vec3 normal{plane[0],plane[1],plane[2]};
        plane={dot(normal,inverse.basis[0])*inverse.scale,
            dot(normal,inverse.basis[1])*inverse.scale,dot(normal,inverse.basis[2])*inverse.scale,
            plane[3]-dot(normal,inverse.position)};
    }
    if(!finite(light.position) || !std::isfinite(light.radius))
        throw FormatError("Path Tracing attachment presentation has a nonfinite frame");
    for(const auto& plane:light.planes)for(float component:plane)
        if(!std::isfinite(component))throw FormatError("Path Tracing attachment presentation has a nonfinite plane");
}
std::vector<Light> source_lights(PhysicsLiveWorldTu15& world,
    std::span<const SourceEffectLightViewRecord> records,const SourceLightPoseProvider& poses) {
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
            const auto count=out.size();
            append(out,origin,value(15),runtime.state);
            if(out.size()!=count) {
                out.back().source_identity=runtime.identity;
                if(!row.first_person_marker)present(world,out.back(),runtime,poses);
            }
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
        out.back().source_identity=runtime.identity;
        if(!row.first_person_marker)present(world,out.back(),runtime,poses);
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
