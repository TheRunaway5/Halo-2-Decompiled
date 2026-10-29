#include "halo2/multiplayer_presentation.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <unordered_map>

namespace halo2 {
namespace {
std::optional<NativeDeathCameraView> sample_camera(const std::optional<NativeDeathCameraView> &before,
    const std::optional<NativeDeathCameraView> &after,float fraction) {
    if(!before || !after || before->camera_mode!=after->camera_mode || fraction==1)return after;
    if(fraction==0)return before;
    auto camera=*after;
    // Share the rigid transform sampler with the body. Linear basis blending
    // would shrink or shear an orbit. Exact endpoints retain source words.
    const auto pose=render_transform(camera_transform(before->position,before->forward,before->up),
        camera_transform(after->position,after->forward,after->up),fraction);
    camera.position=pose.position;camera.forward=pose.basis[0];camera.up=pose.basis[2];
    camera.horizontal_fov=std::lerp(before->horizontal_fov,after->horizontal_fov,fraction);
    camera.aim_displacement=std::lerp(before->aim_displacement,after->aim_displacement,fraction);
    return camera;
}
std::vector<std::uint32_t> body_cuts(const MultiplayerState &previous,const MultiplayerState &current) {
    std::vector<std::uint32_t> cuts;
    for(unsigned i=0;i<current.players.size();++i) {
        const auto &a=previous.players[i],&b=current.players[i];
        if(a.source_weapon!=b.source_weapon || a.secondary_weapon.datum!=b.secondary_weapon.datum)
            for(auto object:{a.source_weapon,b.source_weapon,a.secondary_weapon.datum,b.secondary_weapon.datum})
                if(object!=null_datum)cuts.push_back(object);
        if(a.connected==b.connected && a.dead==b.dead && a.source_player==b.source_player &&
            a.source_unit==b.source_unit && a.source_parent==b.source_parent && a.source_seat==b.source_seat &&
            a.source_teleport_serial==b.source_teleport_serial &&
            length(a.position-b.position)<5)continue;
        // The native root already treats attachment/death as discrete. Apply
        // that same boundary to the actual skin and both carried weapons.
        for(auto object:{a.source_unit,b.source_unit,a.source_weapon,b.source_weapon,
            a.secondary_weapon.datum,b.secondary_weapon.datum})if(object!=null_datum)cuts.push_back(object);
    }
    return cuts;
}
}
void capture_multiplayer_body_poses(SourceWorldInterpolation &history,
    const MultiplayerState &previous,const MultiplayerState &current,bool reset) {
    if(reset || previous.source_body_poses_authoritative!=current.source_body_poses_authoritative) {
        history.reset(current.tick,current.source_body_poses);return;
    }
    history.capture(current.tick,current.source_body_poses,body_cuts(previous,current));
}
void refresh_multiplayer_body_poses(SourceWorldInterpolation &history,
    const MultiplayerState &previous,const MultiplayerState &current) {
    if(previous.source_body_poses_authoritative!=current.source_body_poses_authoritative) {
        history.reset(current.tick,current.source_body_poses);return;
    }
    history.refresh(current.tick,current.source_body_poses,body_cuts(previous,current));
}
MultiplayerState multiplayer_presentation(const MultiplayerState &previous,
    const MultiplayerState &current,float fraction) {
    validate_render_fraction(fraction);
    auto shown=current;
    // A retained snapshot pair can span packet loss. Positions still blend
    // between its endpoints; action clocks require consecutive ticks below.
    if(current.tick<=previous.tick)return shown;
    for(unsigned i=0;i<shown.players.size();++i) {
        const auto &a=previous.players[i],&b=current.players[i];auto &out=shown.players[i];
        // The retained vehicle director also owns death/watch transitions.
        // Its completed death views need the same sampling after moving the
        // local director to the game clock; never bridge a new death owner.
        if(current.tick-previous.tick==1 && a.connected && b.connected && a.dead && b.dead &&
           a.source_player==b.source_player && a.source_previous_unit==b.source_previous_unit &&
           a.source_death_tick==b.source_death_tick)
            out.source_death_camera=sample_camera(a.source_death_camera,b.source_death_camera,fraction);
        if(!a.connected || !b.connected || a.dead || b.dead ||
           a.source_player!=b.source_player || a.source_unit!=b.source_unit ||
           a.source_teleport_serial!=b.source_teleport_serial ||
           a.source_parent!=b.source_parent || a.source_seat!=b.source_seat || length(a.position-b.position)>=5)continue;
        out.position=render_lerp(a.position,b.position,fraction);
        // Across packet loss the observer basis/lens keep their completed
        // endpoint. Carry its measured eye displacement with the interpolated
        // local root instead of leaving only the eye at the newer host root.
        if(current.tick-previous.tick!=1 && b.source_player_camera && fraction!=1)
            out.source_player_camera->position=b.source_player_camera->position+(out.position-b.position);
        out.yaw=render_angle(a.yaw,b.yaw,fraction);
        out.pitch=std::lerp(a.pitch,b.pitch,fraction);out.crouch=std::lerp(a.crouch,b.crouch,fraction);
        if(current.tick-previous.tick==1) {
            out.source_player_camera=sample_camera(a.source_player_camera,b.source_player_camera,fraction);
            out.source_vehicle_camera=sample_camera(a.source_vehicle_camera,b.source_vehicle_camera,fraction);
        }
        const auto &ga=a.grenade_action,&gb=b.grenade_action;
        if(current.tick-previous.tick==1 && ga.animation_active && gb.animation_active &&
           ga.graph==gb.graph && ga.clip==gb.clip && ga.activity==gb.activity &&
           gb.frame>=ga.frame)
            out.grenade_action.frame=std::lerp(ga.frame,gb.frame,fraction);
    }
    const auto sample_roots=[&](const auto &before,auto &after,auto identity) {
        using Object=typename std::remove_cvref_t<decltype(before)>::value_type;
        const auto key=[&](const auto &o){const auto id=identity(o);return (std::uint64_t(id.first)<<32)|id.second;};
        std::unordered_map<std::uint64_t,const Object*> old;old.reserve(before.size());
        for(const auto &o:before)old.emplace(key(o),&o);
        for(auto &b:after) {
            if(identity(b).first==0xffffffffu)continue;
            const auto at=old.find(key(b));if(at==old.end())continue;
            const auto &a=*at->second;
            const auto root=render_transform(source_object_transform(a.position,a.forward,a.up,1,false),
                source_object_transform(b.position,b.forward,b.up,1,false),fraction);
            b.position=root.position;b.forward=root.basis[0];b.up=root.basis[2];
        }
    };
    sample_roots(previous.world_weapons,shown.world_weapons,[](const auto &w){return std::pair{w.handle,w.definition};});
    sample_roots(previous.weapon_projectiles,shown.weapon_projectiles,[](const auto &p){return std::pair{p.datum,p.definition};});
    for(auto &b:shown.grenade_projectiles) {
        if(b.datum==0xffffffffu || b.parent!=0xffffffffu)continue;
        const auto a=std::find_if(previous.grenade_projectiles.begin(),previous.grenade_projectiles.end(),[&](const auto &a){
            return a.datum==b.datum && a.definition==b.definition && a.type==b.type && a.parent==b.parent;
        });
        if(a==previous.grenade_projectiles.end())continue;
        b.position=render_lerp(a->position,b.position,fraction);
    }
    return shown;
}
std::vector<SourceWorldPredictedRoot> predicted_body_roots(const MultiplayerState &previous,
    const MultiplayerState &current,const MultiplayerState &shown,std::span<const unsigned> slots,
    std::span<const std::optional<Vec3>> predicted) {
    const auto same=[](Vec3 a,Vec3 b) {
        return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
            std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
            std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);
    };
    std::vector<SourceWorldPredictedRoot> roots;
    for(unsigned view=0;view<slots.size() && view<predicted.size();++view) {
        const auto slot=slots[view];
        if(slot>=shown.players.size() || !predicted[view])continue;
        const auto &p=shown.players[slot];
        // The camera is drawn from the shown position: the body follows it
        // only while that position is the prediction, on a living, unseated
        // unit (the unit is then its own WORLD root).
        if(!p.connected || p.dead || p.source_unit==null_datum || p.source_parent!=null_datum ||
           p.source_seat!=-1 || !same(p.position,*predicted[view]))continue;
        if(std::any_of(roots.begin(),roots.end(),[&](const auto &root){return root.object==p.source_unit;}))continue;
        roots.push_back({p.source_unit,previous.players[slot].position,current.players[slot].position,p.position});
    }
    return roots;
}
ShadowCasterSphere predicted_body_sphere(std::span<const SourceWorldPredictedRoot> roots,
    std::uint32_t object,const ShadowCasterSphere &current) {
    for(const auto &root:roots)if(root.object==object) {
        // The completed sphere belongs to the current endpoint's origin.
        auto moved=current;moved.center=current.center+(root.predicted-root.current);
        return moved;
    }
    return current;
}
void present_first_person_body_nodes(std::span<Transform> nodes,const MultiplayerState &previous,
    const MultiplayerState &current,const MultiplayerState &shown,unsigned slot,bool paired,float fraction) {
    validate_render_fraction(fraction);
    const auto &before=previous.players.at(slot),&after=current.players.at(slot),&displayed=shown.players.at(slot);
    if(displayed.source_unit!=after.source_unit || displayed.source_unit==null_datum || displayed.dead ||
        displayed.source_parent!=null_datum || displayed.source_seat!=-1)
        throw FormatError("First-person body presentation lost its actual on-foot unit root");
    const auto source=paired?render_lerp(before.position,after.position,fraction):after.position;
    const auto delta=displayed.position-source;
    if(delta.x==0 && delta.y==0 && delta.z==0)return;
    for(auto &node:nodes)node.position=node.position+delta;
}
}
