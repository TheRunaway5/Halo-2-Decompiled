#include "halo2/render_interpolation.hpp"
#include "halo2/math.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/source_shadow.hpp"
#include <glm/gtc/quaternion.hpp>
#include <cstring>

namespace halo2 {
void validate_render_fraction(float f) {
    if(!std::isfinite(f) || f<0 || f>1)throw FormatError("Invalid render interpolation fraction");
}
Vec3 render_lerp(Vec3 a,Vec3 b,float f) {
    validate_render_fraction(f);
    if(f==0)return a;
    if(f==1)return b;
    return {std::lerp(a.x,b.x,f),std::lerp(a.y,b.y,f),std::lerp(a.z,b.z,f)};
}
float render_loop(float a,float b,float period,float f) {
    validate_render_fraction(f);
    if(!std::isfinite(period) || period<=0)throw FormatError("Invalid render interpolation period");
    if(f==0)return a;
    if(f==1 || a==b)return b;
    const float value=a+std::remainder(b-a,period)*f;
    return value-std::floor(value/period)*period;
}
float render_angle(float a,float b,float f) {
    validate_render_fraction(f);
    if(f==0)return a;
    if(f==1 || a==b)return b;
    return a+std::remainder(b-a,6.283185307179586f)*f;
}
Transform render_transform(const Transform &a,const Transform &b,float f) {
    validate_render_fraction(f);
    if(f==0)return a;
    if(f==1 || !std::memcmp(&a,&b,sizeof(a)))return b;
    const auto mirrored=[](const Transform &t) {
        return dot(cross(t.basis[0],t.basis[1]),t.basis[2])<0;
    };
    const bool mirror_a=mirrored(a),mirror_b=mirrored(b);
    const auto rotation=[](const Transform &t,bool mirror) {
        const float handedness=mirror?-1.f:1.f;
        return glm::normalize(glm::quat_cast(glm::mat3(
            glm::vec3(t.basis[0].x,t.basis[0].y,t.basis[0].z),
            glm::vec3(t.basis[1].x*handedness,t.basis[1].y*handedness,t.basis[1].z*handedness),
            glm::vec3(t.basis[2].x,t.basis[2].y,t.basis[2].z))));
    };
    Transform out=b;
    // Keep unchanged rotation words exact, including authored signed zeros.
    // A reflection is a discrete source flag, not a quaternion rotation.
    // Interpolate within the retained handedness and snap its flag transition.
    if(mirror_a==mirror_b && std::memcmp(a.basis.data(),b.basis.data(),sizeof(a.basis))) {
        const auto basis=glm::mat3_cast(glm::slerp(rotation(a,mirror_a),rotation(b,mirror_b),f));
        for(unsigned i=0;i<3;++i)out.basis[i]={basis[i].x,basis[i].y,basis[i].z};
        if(mirror_b)out.basis[1]=out.basis[1]*-1.f;
    }
    out.position=render_lerp(a.position,b.position,f);
    out.scale=std::lerp(a.scale,b.scale,f);
    return out;
}
namespace {
void validate_source_world_poses(std::span<const SourceWorldPose> poses) {
    for(unsigned n=0;n<poses.size();++n) {
        if(poses[n].object==0xffffffffu || poses[n].nodes.empty())
            throw FormatError("WORLD interpolation requires a live full object and current hierarchy");
        for(unsigned before=0;before<n;++before)if(poses[before].object==poses[n].object)
            throw FormatError("WORLD interpolation has duplicate full object ownership");
    }
}
}
void SourceWorldInterpolation::reset(std::uint64_t tick,std::span<const SourceWorldPose> poses) {
    validate_source_world_poses(poses);
    auto next=std::vector<SourceWorldPose>(poses.begin(),poses.end());
    current_=std::move(next);previous_.clear();tick_=tick;initialized_=true;paired_=false;
}
void SourceWorldInterpolation::capture(std::uint64_t tick,std::span<const SourceWorldPose> poses) {
    if(!initialized_ || tick<=tick_ || tick-tick_!=1){reset(tick,poses);return;}
    validate_source_world_poses(poses);
    auto next=std::vector<SourceWorldPose>(poses.begin(),poses.end());
    previous_=std::move(current_);current_=std::move(next);tick_=tick;paired_=true;
}
std::vector<Transform> SourceWorldInterpolation::sample(std::uint32_t object,float f)const {
    validate_render_fraction(f);
    const auto now=std::find_if(current_.begin(),current_.end(),[&](const auto &p){return p.object==object;});
    if(now==current_.end())throw FormatError("WORLD interpolation has no current full object owner");
    if(!paired_ || f==1)return now->nodes;
    const auto old=std::find_if(previous_.begin(),previous_.end(),[&](const auto &p){return p.object==object;});
    if(old==previous_.end() || old->nodes.size()!=now->nodes.size())return now->nodes;
    if(f==0)return old->nodes;
    auto result=now->nodes;
    for(unsigned n=0;n<result.size();++n)result[n]=render_transform(old->nodes[n],now->nodes[n],f);
    return result;
}
void SceneInterpolation::reset(std::uint64_t tick,std::span<const SceneObjects::Object> objects) {
    tick_=tick;initialized_=true;paired_=false;previous_.clear();
    current_.assign(objects.begin(),objects.end());
}
void SceneInterpolation::capture(std::uint64_t tick,std::span<const SceneObjects::Object> objects) {
    if(!initialized_ || tick<=tick_ || tick-tick_!=1){reset(tick,objects);return;}
    previous_.swap(current_);current_.assign(objects.begin(),objects.end());
    tick_=tick;paired_=true;
}
Transform scene_object_transform(const SceneObjects::Object &o) {
    if(o.solved_transform)return *o.solved_transform;
    const auto basis=authored_basis(o.rotation);
    return source_object_transform(o.position,basis.forward,basis.up,o.scale,false);
}
Transform SceneInterpolation::sample(const SceneObjects::Object &current,float f)const {
    validate_render_fraction(f);
    const auto now=scene_object_transform(current);
    if(!paired_ || current.handle==0xffffffffu || !current.visible || current.bsp_hidden)return now;
    const auto old=std::find_if(previous_.begin(),previous_.end(),[&](const auto &o) {
        return o.handle==current.handle && o.placement==current.placement &&
            o.created_tick==current.created_tick && o.bsp==current.bsp && o.visible && !o.bsp_hidden;
    });
    return old==previous_.end()?now:render_transform(scene_object_transform(*old),now,f);
}
std::vector<Transform> SceneInterpolation::sample_nodes(const SceneObjects::Object &current,float f)const {
    validate_render_fraction(f);
    if(!current.solved_nodes)throw FormatError("Scene WORLD interpolation requires the current source hierarchy");
    const auto &now=*current.solved_nodes;
    if(!paired_ || current.handle==0xffffffffu || !current.visible || current.bsp_hidden || f==1)return now;
    const auto old=std::find_if(previous_.begin(),previous_.end(),[&](const auto &o) {
        return o.handle==current.handle && o.placement==current.placement &&
            o.created_tick==current.created_tick && o.bsp==current.bsp && o.visible && !o.bsp_hidden;
    });
    if(old==previous_.end() || !old->solved_nodes || old->solved_nodes->size()!=now.size())return now;
    if(f==0)return *old->solved_nodes;
    auto result=now;
    for(unsigned n=0;n<result.size();++n)result[n]=render_transform(old->solved_nodes->at(n),now[n],f);
    return result;
}
}
