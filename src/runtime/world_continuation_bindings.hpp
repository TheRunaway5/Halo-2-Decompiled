#pragma once
#include "halo2/content.hpp"
#include "halo2/scene.hpp"
#include "halo2/transform.hpp"
#include "halo2/native_grenade_constants.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/native_melee_animation.hpp"
#include "halo2/source_runtime_resources.hpp"
#include <cmath>

namespace halo2::world_continuation {
std::array<std::byte,32> prop_definitions(const LevelContent &);
std::array<std::byte,32> scenery_definitions(const LevelContent &);
std::array<std::byte,32> device_definitions(const LevelContent &);
std::array<std::byte,32> bsp_geometry(const BspContent &);
std::array<std::byte,32> biped_definition(const BipedDefinition &);
std::array<std::byte,32> unit_interaction_definitions(const LevelContent &);
std::array<std::byte,32> melee_binding(const Cache &,const NativeMeleeConstants &,float,
    const std::map<std::uint32_t,int> &,const std::map<std::uint32_t,std::uint32_t> &);
std::array<std::byte,32> melee_binding(const SourceRuntimeResources::Identity &,const NativeMeleeConstants &,float,
    const std::map<std::uint32_t,int> &,const std::map<std::uint32_t,std::uint32_t> &);
std::array<std::byte,32> triangles(std::span<const CollisionTriangle>);
std::array<std::byte,32> script_program(const ScriptProgram &);
std::array<std::byte,32> presentation_content(const LevelContent &);
std::array<std::byte,32> grenade_binding(const Cache &,std::uint32_t,NativeGrenadeConstants);
std::array<std::byte,32> grenade_unit(const RenderModel &,std::span<const ObjectAttachmentMarker>,std::span<const std::uint8_t>,float);
std::array<std::byte,32> animation_clip(const AnimationClip &);
std::array<std::byte,32> grenade_clips(const AnimationClip &,const std::optional<AnimationClip> &);
inline bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline bool finite(const Transform &t){
    if(!std::isfinite(t.scale) || !finite(t.position))return false;
    for(const auto &axis:t.basis)if(!finite(axis))return false;
    return true;
}
inline const SceneObjects::Object &scene_owner(const SceneObjects::AuthorityCheckpoint &scene,
    std::uint32_t handle,std::size_t placement) {
    const auto at=scene.slots.find(handle);
    if(scene.version!=1 || handle==null_datum || !(handle>>16) || (handle&65535u)>=2048 ||
        at==scene.slots.end() || at->second>=scene.objects.size() ||
        scene.objects[at->second].handle!=handle || scene.objects[at->second].placement!=placement)
        throw FormatError("World continuation lost its admitted scene generation");
    return scene.objects[at->second];
}
}
