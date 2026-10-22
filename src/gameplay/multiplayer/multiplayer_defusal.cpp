#include "halo2/multiplayer.hpp"
#include "halo2/math.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <algorithm>

namespace halo2 {
BombDefusalPlayer MultiplayerMatch::defusal_player(unsigned id,const Controls &c)const {
  const auto &p=players_.at(id);const auto &u=unit(id);
  const bool active=p.connected && !(p.source_flags&2u);
  BombDefusalPlayer result{p.source_datum,p.source_membership,active,
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
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected && !(players_[i].source_flags&2u)) {
    auto &p=players_[i];auto &u=unit_mutable(i);
    if(simulation_.grenade_sandbox_)simulation_.grenade_sandbox_->cancel_unit(u);
    if(p.source_current_object!=null_datum) {
      u.vitality.dead=true;u.vitality.body=0;u.state=Unit::Dead;
      finish_death(u,p,std::uint32_t(simulation_.ticks()));
    }
  }
  for(const auto &p:simulation_.projectiles_)if(p.source_datum!=null_datum)
    simulation_.object_lifetimes_.release(p.source_datum);
  simulation_.projectiles_.clear();
  if(simulation_.grenade_sandbox_)simulation_.grenade_sandbox_->discard_projectiles(
      [this](auto h){simulation_.object_lifetimes_.release(h);});
  for(const auto &pickup:weapon_pickups_)if(pickup.weapon && !simulation_.object_lifetimes_.header(pickup.handle).empty())
    simulation_.object_lifetimes_.release(pickup.handle);
  weapon_pickups_.clear();
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected && !(players_[i].source_flags&2u)) {
    source_initialize_player_lives(players_[i].source_lives_remaining,1,true,0);
    if(!spawn(i,true))throw FormatError("Bomb Defusal could not spawn a round participant");
  }
}
void MultiplayerMatch::update_defusal(const std::array<Controls,multiplayer_capacity> &controls) {
  std::array<BombDefusalPlayer,multiplayer_capacity> roster;
  for(unsigned i=0;i<roster.size();++i)roster[i]=defusal_player(i,controls[i]);
  const auto before=defusal_->scores;
  if(bomb_defusal_tick(*defusal_,*rules_.defusal,roster))reset_defusal_players();
  for(unsigned team=0;team<2;++team)if(before[team]!=defusal_->scores[team]) {
    team_scores_[team]=std::int16_t(defusal_->scores[team]);
    end_stats_.teams[team].fields[0]=team_scores_[team];
    round_lifecycle_.flags|=0x20;
  }
  if(defusal_->phase==BombDefusalPhase::complete)end_match(-1,defusal_->match_winner);
}
} // namespace halo2
