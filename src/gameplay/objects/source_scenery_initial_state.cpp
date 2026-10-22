#include "halo2/source_scenery_initial_state.hpp"
#include <bit>
#include <utility>

namespace halo2 {
namespace {
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
bool same(const Transform& a,const Transform& b){
    if(!same(a.scale,b.scale)||!same(a.position,b.position))return false;
    for(unsigned i=0;i<3;++i)if(!same(a.basis[i],b.basis[i]))return false;
    return true;
}
}
SourceSceneryAnimationRefresh source_scenery_animation_refresh(
    const SourceSceneryAnimationControllerState& input){
    if(input.root_header_flags&0x80u)
        throw FormatError("attached scenery refresh requires the original root owner");
    SourceSceneryAnimationRefresh result{false,input.root_header_flags};
    if(!input.ready || input.animation_frames<=1)
        return result;
    result.multi_frame=true;
    if(result.root_header_flags&1u)result.root_header_flags|=6u;
    return result;
}
SourceSceneryAnimationRefresh source_scenery_animation_refresh(
    const SourceSceneryAnimationRefreshInput& input){
    const bool ready=input.controller_offset!=-1 && input.graph_datum!=0xffffffffu &&
        input.animation_graph_datum!=0xffffffffu && std::int16_t(input.animation_binding>>16)!=-1;
    return source_scenery_animation_refresh(SourceSceneryAnimationControllerState{
        ready,input.animation_frames,input.root_header_flags});
}
const SourceSceneryInitialQueryState& SourceSceneryInitialState::capture(std::uint32_t handle,
    std::uint32_t definition,const ShadowCasterDefinition& bounds,
    const Transform& node0,float scale){
    if(handle==0xffffffffu || definition==0xffffffffu || entries_.contains(handle))
        throw FormatError("invalid or duplicate source scenery constructor identity");
    Entry entry;entry.definition=definition;entry.initial_world_node0=node0;
    entry.initial_object_scale=scale;
    entry.state.sphere=source_shadow_sphere(bounds,node0,scale);
    // All 278 automatic Slayer type6 prefixes: velocity=0, query bit13 clear,
    // physics datum NONE/C0 bit6 clear and C1 bit0 frozen before B8890.
    return entries_.emplace(handle,std::move(entry)).first->second.state;
}
bool SourceSceneryInitialState::contains(std::uint32_t handle)const{return entries_.contains(handle);}
const SourceSceneryInitialQueryState& SourceSceneryInitialState::retained(std::uint32_t handle,
    std::uint32_t definition,const Transform& node0,float scale,bool physics_advanced)const{
    const auto found=entries_.find(handle);
    if(found==entries_.end() || found->second.definition!=definition)
        throw FormatError("source scenery constructor cache identity is unresolved");
    const auto& entry=found->second;
    if(physics_advanced || !same(entry.initial_world_node0,node0) || !same(entry.initial_object_scale,scale))
        throw FormatError("source scenery bounds require the later pose/physics/cache producer");
    return entry.state;
}
const SourceSceneryInitialQueryState& SourceSceneryInitialState::refresh_cached_bounds(
    std::uint32_t handle,std::uint32_t definition,const ShadowCasterDefinition& bounds,
    const Transform& node0,float scale,bool admitted){
    const auto found=entries_.find(handle);
    if(found==entries_.end() || found->second.definition!=definition)
        throw FormatError("source scenery bounds refresh identity is unresolved");
    if(!admitted)
        throw FormatError("source scenery bounds refresh requires an admitted pose/bounds update");
    auto& entry=found->second;
    entry.state.sphere=source_shadow_sphere(bounds,node0,scale);
    entry.initial_world_node0=node0;entry.initial_object_scale=scale;
    return entry.state;
}
void SourceSceneryInitialState::destroy(std::uint32_t handle){entries_.erase(handle);}
}
