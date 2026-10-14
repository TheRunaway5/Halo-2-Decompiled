#include "halo2/multiplayer_presentation.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/math.hpp"
#include <unordered_map>

namespace halo2 {
void capture_multiplayer_body_poses(SourceWorldInterpolation &history,
    const MultiplayerState &previous,const MultiplayerState &current,bool reset) {
    if(reset || previous.source_body_poses_authoritative!=current.source_body_poses_authoritative) {
        history.reset(current.tick,current.source_body_poses);return;
    }
    std::vector<std::uint32_t> cuts;
    for(unsigned i=0;i<current.players.size();++i) {
        const auto &a=previous.players[i],&b=current.players[i];
        if(a.connected==b.connected && a.dead==b.dead && a.source_player==b.source_player &&
            a.source_unit==b.source_unit && a.source_parent==b.source_parent && a.source_seat==b.source_seat)continue;
        // The native root already treats attachment/death as discrete. Apply
        // that same boundary to the actual skin and both carried weapons.
        for(auto object:{a.source_unit,b.source_unit,a.source_weapon,b.source_weapon,
            a.secondary_weapon.datum,b.secondary_weapon.datum})if(object!=null_datum)cuts.push_back(object);
    }
    history.capture(current.tick,current.source_body_poses,cuts);
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
        if(!a.connected || !b.connected || a.dead || b.dead ||
           a.source_player!=b.source_player || a.source_unit!=b.source_unit ||
           a.source_parent!=b.source_parent || a.source_seat!=b.source_seat || length(a.position-b.position)>=5)continue;
        out.position=render_lerp(a.position,b.position,fraction);
        out.yaw=render_angle(a.yaw,b.yaw,fraction);
        out.pitch=std::lerp(a.pitch,b.pitch,fraction);out.crouch=std::lerp(a.crouch,b.crouch,fraction);
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
}
