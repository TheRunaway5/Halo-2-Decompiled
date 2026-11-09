#include "halo2/multiplayer.hpp"
#include "halo2/math.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include <algorithm>
#include <set>

namespace halo2 {
namespace {
void detach_bomb(MultiplayerObjectRuntime &objects,std::uint32_t bomb) {
  if(objects.object_parent(bomb)==null_datum)return;
  // HS76's original2ABA1F validates the actual parent before BBAC0.
  // The getter above observes that same full source parent generation.
  auto &world=objects.tu15_world();auto call=world.prepare_source_call();
  call.gp[3]=bomb;world.run_actual(0xbbac0u,call);
  if(objects.object_parent(bomb)!=null_datum)
    throw FormatError("Original object detach retained the custom bomb parent");
}
}
BombDefusalPlayer MultiplayerMatch::defusal_player(unsigned id,const Controls &c)const {
  const auto &p=players_.at(id);const auto &u=unit(id);
  const bool active=p.connected && !(p.source_flags&2u);
  // A pending player retains its requested custom team for the next round's
  // roster. Native membership/profile remain accepted NONE until that reset.
  const auto team=p.source_membership==-1 && p.source_profile_request?
      std::bit_cast<std::int8_t>((*p.source_profile_request)[0x7c]):p.source_membership;
  BombDefusalPlayer result{p.source_datum,team,active,
      active && p.source_current_object!=null_datum && u.exists && !u.vitality.dead,
      c.pickup,std::abs(c.forward)>.05f || std::abs(c.right)>.05f ||
          u.body.velocity.x*u.body.velocity.x+u.body.velocity.y*u.body.velocity.y>.04f,
      u.body.grounded,u.body.position};
  if(defusal_ && (defusal_->phase==BombDefusalPhase::planted || defusal_->carrier==null_datum)) {
    auto eye=u.body.position;eye.z+=.4f;
    auto target=defusal_->bomb_position;target.z+=.1f;
    const auto delta=target-eye;const auto distance=length(delta);
    if(distance>.05f)result.unobstructed=!simulation_.collision().raycast(eye,delta/distance,distance-.05f);
  }
  return result;
}
void MultiplayerMatch::reset_defusal_players() {
  // Round transitions do not count deaths or kills. Retire the old body using
  // normal native ownership, then construct a new full generation/loadout.
  if(simulation_.source_physics_movement_)detach_bomb(*source_object_world_,defusal_bomb_);
  std::set<std::uint32_t> native_items;
  std::set<std::uint32_t> authored;
  if(simulation_.source_physics_movement_) {
    for(const auto &object:simulation_.scene_.objects())authored.insert(object.handle);
    for(const auto &object:source_object_world_->live_objects())
      if(object.object!=defusal_bomb_ && !authored.contains(object.object) &&
          (object.type==2 || object.type==5))native_items.insert(object.object);
  }
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected && !(players_[i].source_flags&2u)) {
    auto &p=players_[i];auto &u=unit_mutable(i);
    if(simulation_.grenade_sandbox_ && !simulation_.source_physics_movement_)
      simulation_.grenade_sandbox_->cancel_unit(u);
    const auto previous=p.source_current_object!=null_datum?p.source_current_object:
        simulation_.source_physics_movement_?u.source_datum:null_datum;
    if(previous!=null_datum) {
      if(simulation_.source_physics_movement_ && !simulation_.object_lifetimes_.header(previous).empty())
        for(const auto weapon:source_object_world_->player_equipment(previous).weapons)
          if(weapon!=null_datum && !authored.contains(weapon))native_items.insert(weapon);
      if(p.source_current_object!=null_datum) {
        u.vitality.dead=true;u.vitality.body=0;u.state=Unit::Dead;
        finish_death(u,p,std::uint32_t(simulation_.ticks()));
      }
      if(simulation_.source_physics_movement_) {
        if(!simulation_.object_lifetimes_.header(previous).empty())source_object_world_->destroy_object(previous);
        simulation_.object_lifetimes_.observe_source_retirement(previous);
        corpses_.erase(previous);retained_units_.erase(previous);
        u={};u.exists=false;u.vitality.dead=true;
      }
    }
  }
  for(const auto &p:simulation_.projectiles_)if(p.source_datum!=null_datum) {
    if(simulation_.source_physics_movement_) {
      if(!simulation_.object_lifetimes_.header(p.source_datum).empty())source_object_world_->destroy_object(p.source_datum);
    }
    else simulation_.object_lifetimes_.release(p.source_datum);
  }
  simulation_.projectiles_.clear();
  if(simulation_.grenade_sandbox_)simulation_.grenade_sandbox_->discard_projectiles(
      [this](auto h){
        if(simulation_.source_physics_movement_) {
          if(!simulation_.object_lifetimes_.header(h).empty())source_object_world_->destroy_object(h);
        } else simulation_.object_lifetimes_.release(h);
      });
  for(const auto &pickup:weapon_pickups_)if(pickup.weapon && !simulation_.object_lifetimes_.header(pickup.handle).empty()) {
    if(simulation_.source_physics_movement_) {
      if(!authored.contains(pickup.handle))source_object_world_->destroy_object(pickup.handle);
    }
    else simulation_.object_lifetimes_.release(pickup.handle);
  }
  weapon_pickups_.clear();
  if(simulation_.source_physics_movement_) {
    for(const auto item:native_items)if(!simulation_.object_lifetimes_.header(item).empty()) {
      source_object_world_->destroy_object(item);
      simulation_.object_lifetimes_.observe_source_retirement(item);
    }
    for(auto &player:players_)if(player.connected && !(player.source_flags&2u) && player.source_membership==-1) {
      if(!player.source_profile_request)
        throw FormatError("Pending Defusal participant lost its requested team input");
      const auto requested=std::bit_cast<std::int8_t>((*player.source_profile_request)[0x7c]);
      const DatumPool &pool=player_pool_;const auto raw=Bytes(std::as_bytes(pool.lookup(player.source_datum)));
      if(requested<0 || requested>1 || raw.u32(0x2c)!=null_datum || (raw.u16(0xc0)&255u)!=0xffu)
        throw FormatError("Defusal round participation requires its actual pending native owner");
      const auto accepted=source_object_world_->update_player_configuration(player.source_datum,*player.source_profile_request);
      if(std::bit_cast<std::int8_t>(accepted[0x7c])!=requested)
        throw FormatError("Original player configuration refused the requested Defusal round team");
      player.source_configuration=accepted;player.source_membership=requested;player.source_strength=accepted[0x7d];
    }
    // Original round startup reinitializes its actual player-life inputs;
    // only the custom overlay awards round points and chooses round timing.
    source_object_world_->tu15_world().call_actual(0x15afb0u);
    source_engine_started_=true;
  }
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected && !(players_[i].source_flags&2u)) {
    source_initialize_player_lives(players_[i].source_lives_remaining,1,true,0);
    if(!spawn(i,true))throw FormatError("Bomb Defusal could not spawn a round participant");
  }
  if(simulation_.source_physics_movement_)update_first_person_views();
}
void MultiplayerMatch::update_defusal(const std::array<Controls,multiplayer_capacity> &controls) {
  if(simulation_.source_physics_movement_ && defusal_->carrier==null_datum)
    defusal_->bomb_position=source_object_world_->object_transform(defusal_bomb_).position;
  const auto previous_carrier=defusal_->carrier;
  const auto previous_phase=defusal_->phase;
  std::array<BombDefusalPlayer,multiplayer_capacity> roster;
  for(unsigned i=0;i<roster.size();++i)roster[i]=defusal_player(i,controls[i]);
  const auto before=defusal_->scores;
  if(bomb_defusal_tick(*defusal_,*rules_.defusal,roster))reset_defusal_players();
  if(simulation_.source_physics_movement_) {
    auto &objects=*source_object_world_;auto &world=objects.tu15_world();
    const auto *carrier=player_from_datum(defusal_->carrier);
    const auto parent=carrier?carrier->source_current_object:null_datum;
    if(parent!=null_datum) {
      if(objects.object_parent(defusal_bomb_)!=parent) {
        detach_bomb(objects,defusal_bomb_);
        // Empty marker inputs use original BAD60's root fallback. The
        // original hierarchy then follows the carrier without pose writes.
        source_campaign_cinematic_attach_tu15(world,parent,0u,defusal_bomb_,0u);
        if(objects.object_parent(defusal_bomb_)!=parent)
          throw FormatError("Original hierarchy rejected the custom bomb carrier");
      }
    } else if(previous_carrier!=defusal_->carrier || previous_phase!=defusal_->phase) {
      detach_bomb(objects,defusal_bomb_);
      world.set_object_position(defusal_bomb_,defusal_->bomb_position);
    }
  }
  for(unsigned team=0;team<2;++team)if(before[team]!=defusal_->scores[team]) {
    team_scores_[team]=std::int16_t(defusal_->scores[team]);
    end_stats_.teams[team].fields[0]=team_scores_[team];
    round_lifecycle_.flags|=0x20;
  }
  if(defusal_->phase==BombDefusalPhase::complete)end_match(-1,defusal_->match_winner);
}
} // namespace halo2
