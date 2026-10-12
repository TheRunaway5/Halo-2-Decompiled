#include "halo2/source_prop_initial_state.hpp"
#include <bit>

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
const SourcePropInitialQueryState& SourcePropInitialState::capture(std::uint32_t handle,
    std::uint32_t definition,const ShadowCasterDefinition& bounds,
    const Transform& node0,float scale){
    if(handle==0xffffffffu || definition==0xffffffffu || entries_.contains(handle))
        throw FormatError("invalid or duplicate source prop constructor identity");
    Entry entry;entry.definition=definition;entry.initial_world_node0=node0;
    entry.initial_object_scale=scale;
    entry.state.sphere=source_shadow_sphere(bounds,node0,scale);
    // D5560 forwards force1 through BF0F0/D5060. Type11 is in0x1883,
    // so request+18bit3 becomes B7B40's object+C1bit0.
    entry.state.frozen=true;
    // BC280 zeroes the object. All 415 untouched type11 factory prefixes reach
    // their first BDEF0 with velocity=0 and D1850's runtime bit13 clear.
    return entries_.emplace(handle,std::move(entry)).first->second.state;
}
bool SourcePropInitialState::contains(std::uint32_t handle)const{return entries_.contains(handle);}
const SourcePropInitialQueryState& SourcePropInitialState::retained(std::uint32_t handle,
    std::uint32_t definition,const Transform& node0,float scale,bool physics_advanced)const{
    const auto found=entries_.find(handle);
    if(found==entries_.end() || found->second.definition!=definition)
        throw FormatError("source prop constructor cache identity is unresolved");
    const auto& entry=found->second;
    if(physics_advanced || !same(entry.initial_world_node0,node0) || !same(entry.initial_object_scale,scale))
        throw FormatError("source prop bounds require the later physics/cache producer");
    return entry.state;
}
void SourcePropInitialState::destroy(std::uint32_t handle){entries_.erase(handle);}
}
