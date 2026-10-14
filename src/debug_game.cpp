#include "halo2/debug_game.hpp"
#include "halo2/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace halo2 {
const Unit& Simulation::debug_player(unsigned slot) const {
    if(slot==0)return player_;
    if(slot>=16 || slot>actors_.size() || !actors_[slot-1].controlled)
        throw FormatError("debug player slot is not a controlled participant");
    return actors_[slot-1];
}
Unit& Simulation::debug_player_mutable(unsigned slot) {
    return const_cast<Unit&>(std::as_const(*this).debug_player(slot));
}
const DebugPlayerSettings& Simulation::debug_settings_for(const Unit& unit) const {
    if(&unit==&player_)return debug_settings_.players[0];
    for(unsigned i=0;i<actors_.size() && i<15;++i)
        if(&unit==&actors_[i] && unit.controlled)return debug_settings_.players[i+1];
    static const DebugPlayerSettings disabled;
    return disabled;
}
void Simulation::debug_heal(unsigned slot) {
    NativeAuthorityBoundary::Call active(authority_calls_);
    auto& u=debug_player_mutable(slot);
    if(!u.exists || u.vitality.dead)throw FormatError("restore vitality needs a living player");
    ++authority_calls_.mutations;
    u.vitality.body=u.definition.vitality.health>0?u.definition.vitality.body_max:0;
    u.vitality.shield=u.definition.vitality.shield>0?1.f:0;
    u.vitality.body_stun=u.vitality.shield_stun=0;
}
void Simulation::debug_teleport(unsigned slot,const Spawn& spawn) {
    NativeAuthorityBoundary::Call active(authority_calls_);
    if(spawn.bsp!=bsp_ || !std::isfinite(spawn.position.x) || !std::isfinite(spawn.position.y) ||
       !std::isfinite(spawn.position.z) || !std::isfinite(spawn.facing))
        throw FormatError("debug teleport needs an authored spawn in the loaded BSP");
    auto& u=debug_player_mutable(slot);
    if(!u.exists || u.vitality.dead)throw FormatError("teleport needs a living player");
    ++authority_calls_.mutations;
    u.body.position=spawn.position;u.body.velocity={};u.body.grounded=false;
    u.yaw=spawn.facing;u.pitch=0;
    if(slot==0)player_pitch_=0;
    u.route.clear();u.waypoint=0;
}
void Simulation::debug_refill_weapon(Unit& unit) {
    const auto it=content_.weapons.find(unit.weapon.definition);
    if(it==content_.weapons.end())return;
    const auto& definition=it->second;
    auto& weapon=unit.weapon;
    weapon.loaded=definition.capacity;weapon.reserve=std::max(0,definition.reserve);
    if(multiplayer_weapon_steps_ && definition.smg) {
        weapon.smg.loaded=definition.smg->capacity;
        weapon.smg.reserve=static_cast<std::int16_t>(std::clamp(weapon.reserve,0,32767));
        weapon.loaded=weapon.smg.loaded;weapon.reserve=weapon.smg.reserve;
    }
}
DebugGameObservation observe_debug_game(const Simulation& simulation,std::span<const Spawn> spawns) {
    DebugGameObservation o;
    o.mode="Local campaign/workbench";o.limitation="Local player roster is selected before launching a match.";
    o.available=o.ai_available=o.snapshots_available=true;
    o.tick=simulation.ticks();o.checkpoint_available=simulation.has_checkpoint();
    const auto& p=simulation.player();
    o.players.push_back({0,"Player 1",p.body.position,p.vitality.body,p.vitality.shield,p.weapon.loaded,p.weapon.reserve,p.vitality.dead});
    for(unsigned i=0;i<spawns.size();++i)if(spawns[i].bsp==simulation.bsp())
        o.teleports.push_back({"Spawn "+std::to_string(i)+" (BSP "+std::to_string(spawns[i].bsp)+")",spawns[i]});
    o.snapshots=simulation.debug_snapshot_names();
    return o;
}
void apply_debug_request(Simulation& simulation,const DebugGameRequest& request,std::span<const Spawn> spawns) {
    switch(request.kind) {
    case DebugGameRequest::Heal:simulation.debug_heal(request.player);break;
    case DebugGameRequest::Teleport: {
        const auto observation=observe_debug_game(simulation,spawns);
        if(request.index>=observation.teleports.size())throw FormatError("debug spawn selection expired");
        simulation.debug_teleport(request.player,observation.teleports[request.index].spawn);break;
    }
    case DebugGameRequest::SaveCheckpoint:simulation.request_checkpoint();break;
    case DebugGameRequest::RevertCheckpoint:simulation.request_revert();break;
    case DebugGameRequest::SaveSnapshot:simulation.debug_save_snapshot(request.name);break;
    case DebugGameRequest::LoadSnapshot:simulation.debug_load_snapshot(request.name);break;
    case DebugGameRequest::DeleteSnapshot:simulation.debug_delete_snapshot(request.name);break;
    }
}
} // namespace halo2
