#include "halo2/multiplayer.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/native_weapon_interaction.hpp"
#include "halo2/math.hpp"
#include "halo2/pickup_visibility.hpp"
#include "halo2/player_inventory_admission.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <limits>

namespace halo2 {
void MultiplayerMatch::configure_world_weapon_effect_owner(
    std::function<std::uint32_t(const WeaponEffectInput &)> owner) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  simulation_.world_weapon_effect_hook_=std::move(owner);
}
void MultiplayerMatch::configure_weapon_pickups() {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(weapon_pickups_configured_ || simulation_.ticks()!=0)
    throw FormatError("Native weapon pickups must be configured once before stepping");
  weapon_pickups_configured_=true;
  if(!rules_.defusal)simulation_.configure_vehicle_rows(rules_.source_engine);
  simulation_.configure_vehicle_allegiance([this](const Unit &a,const Unit &b){
    if(a.source_player==b.source_player)return true;
    const auto *pa=player_from_datum(a.source_player),*pb=player_from_datum(b.source_player);
    return pa && pb && rules_.team_mode && pa->source_membership>=0 &&
        pa->source_membership==pb->source_membership;
  });
  pickup_rows_.resize(content_.netgame_equipment.size());
}
void MultiplayerMatch::drop_weapon(Unit &u,int slot) {
  native_drop_weapon(u,slot,weapon_pickups_,std::uint32_t(simulation_.ticks()),
      [this](Unit &unit,WeaponState &weapon){simulation_.clear_weapon_for_holster(unit,weapon);});
}
void MultiplayerMatch::drop_secondary_weapon(Unit &u) {
  if(u.source_inventory.current[1]>=0)drop_weapon(u,u.source_inventory.current[1]);
}
void MultiplayerMatch::drop_carried_weapons(Unit &u) {
  if(!weapon_pickups_configured_)return;
  for(int slot=0;slot<4;++slot)drop_weapon(u,slot);
}
NativeActionPrompt MultiplayerMatch::action_prompt(unsigned id)const {
  NativeActionPrompt result;
  if(!weapon_pickups_configured_ || id>=players_.size() || !players_[id].connected ||
      players_[id].source_current_object==null_datum || finished_)return result;
  const auto &u=unit(id);
  NativeWeaponPromptChoice selected{};
  if(simulation_.source_physics_movement_) {
    const auto target=source_object_world_->tu15_world().player_action_target(players_[id].source_datum,u.source_datum);
    if(target && (target->action==1 || target->action==7) && target->object!=null_datum) {
      try {
        const auto definition=source_object_world_->object_definition(target->object);
        if(content_.weapons.contains(definition))return {*target,definition,0};
      }catch(const FormatError&){} // A completed source phase may retire that generation.
      return {};
    }
    // Other existing control/unit presentation keeps its own typed producer.
    // A source weapon never falls back to legacy pickup registrations.
  }else selected=native_weapon_action_prompt(u,content_,simulation_.collision(),weapon_pickups_,
      std::uint32_t(simulation_.ticks()));
  const auto control=native_control_action_prompt(u,content_,simulation_.devices(),selected.prompt,selected.root);
  return simulation_.unit_action_prompt(u,control,selected.root);
}
void MultiplayerMatch::step_weapon_pickups(const std::array<Controls,multiplayer_capacity> &controls) {
  std::vector<std::pair<Unit*,std::uint32_t>> ready_animations;
  if(!weapon_pickups_configured_)return;
  const auto tick=std::uint32_t(simulation_.ticks());
  for(unsigned row=0;row<pickup_rows_.size();++row) {
    if(rules_.defusal)break; // Fixed round loadouts; dropped weapons remain usable.
    const auto &d=content_.netgame_equipment[row];
    if(d.classification || d.items.size()!=1 || source_netgame_equipment_selector(d,true,rules_.source_engine)<0)continue;
    const auto definition=std::find_if(d.items.begin(),d.items.end(),[&](const auto &item) {
      return item.group==0x77656170u && content_.weapons.contains(item.definition) &&
          !content_.weapons.at(item.definition).first_person_models.empty() &&
          content_.weapons.at(item.definition).first_person_models.front().model!=null_datum &&
          content_.weapons.at(item.definition).first_person_models.front().animation!=null_datum;
    });
    if(definition==d.items.end())continue;
    auto &state=pickup_rows_[row];
    const auto item=std::find_if(weapon_pickups_.begin(),weapon_pickups_.end(),
        [&](const auto &p){return p.handle==state.object;});
    const bool taken=item==weapon_pickups_.end() || !item->weapon;
    const auto position=item==weapon_pickups_.end()?d.position:item->position;
    if(row>=100)continue; // Source row timer storage supports 100 entries.
    if(netgame_weapon_timer(state,row,std::int32_t(tick),30,true,false,d.position,position,
        taken?1u:0u,d.respawn_seconds,0)) {
      const bool source_birth=source_object_world_ && source_object_world_->owns_physics();
      const auto handle=source_birth?source_object_world_->create_netgame_object(row):allocate_object(2);
      if(handle==null_datum)continue;
      const auto selected=source_birth?source_object_world_->object_definition(handle):definition->definition;
      const auto &w=content_.weapons.at(selected);
      const auto basis=authored_basis(d.rotation);
      weapon_pickups_.push_back({handle,w.datum,tick,d.position,basis.forward,basis.up,native_weapon_state(w,handle)});
      if(source_birth)simulation_.object_lifetimes_.observe_source_birth(handle);
      else simulation_.object_lifetimes_.publish_native(handle,w.datum);
      netgame_weapon_spawn_result(state,handle);
    }
  }
  for(unsigned id=0;id<players_.size();++id) {
    if(!players_[id].connected || players_[id].source_current_object==null_datum)continue;
    auto &u=unit_mutable(id);if(u.vitality.dead)continue;
    if(rules_.custom==CustomSlayer::infection && (infection_zombies_&(1u<<id)))continue;
    // The original14FEC0/1522E0 pipeline owns TU1.5 inventory and unit
    // actions over the same bodies. Native presentation observes its result.
    if(source_object_world_ && source_object_world_->owns_physics())continue;
    const auto &c=controls[id];auto &inventory=u.source_inventory;
    std::array<std::uint32_t,2> before{u.weapon.source_datum,
        native_hand_weapon(u,1)?native_hand_weapon(u,1)->source_datum:null_datum};
    const auto begin_ready=[&] {
      for(unsigned h=0;h<2;++h)if(auto *w=native_hand_weapon(u,h);w && w->source_datum!=before[h]) {
        const auto &d=content_.weapons.at(w->definition);
        w->ready_countdown=source_weapon_ready_begin(w->shader,d.ready_seconds,30);w->ready_clock=true;
        if(d.ready_effect!=null_datum && simulation_.weapon_effect_hook_)
          simulation_.weapon_effect_hook_(u,{d.ready_effect,w->source_datum,null_datum,0,0});
        ready_animations.emplace_back(&u,w->source_datum);
        before[h]=w->source_datum;
      }
    };
    auto actions=source_pickup_buttons(c.pickup,c.pickup?c.pickup_milliseconds:0,
        c.dual_wield,c.dual_wield?c.dual_wield_milliseconds:0,source_pickup_hold_threshold(content_.pickup_hold_seconds));
    std::uint32_t gameplay=0;
    const bool consumed_dual=u.source_player_actions.flags&0x20u;
    source_player_action_edges(u.source_player_actions,gameplay,actions);
    const bool pickup=actions&2u, dual_wield=actions&0x20u;
    if(c.grenade && grenade_ready_type(u.grenade_current,u.grenades)!=-1 &&
        !(u.source_action_word&0x400001u))drop_secondary_weapon(u);
    if(u.source_action_word&0x400000u)continue;
    const auto release_target=c.switch_weapon && dual_wield?action_prompt(id):NativeActionPrompt{};
    const bool admitted_dual=dual_wield && (release_target.target.action==1 || release_target.target.action==7) &&
        (release_target.target.index&4);
    // Native polling can deliver a complete hold and its release together.
    // Preserve the admitted transfer in that case; a previously consumed
    // hold also owns the release before150DD0 clears its secondary latch.
    if(c.switch_weapon && !consumed_dual && !admitted_dual) {
      drop_secondary_weapon(u);
      for(int step=1;step<4;++step) {
        const int slot=(int(inventory.current[0])+step+4)%4;
        if(native_select_weapon(u,slot,tick,[this](Unit &unit,WeaponState &weapon){simulation_.clear_weapon_for_holster(unit,weapon);}))break;
      }
    }
    begin_ready();
    if(!pickup && !dual_wield)continue;
    const auto prompt=action_prompt(id);
    if(pickup && u.source_parent!=null_datum) {simulation_.take_unit_action(u,{{0,u.source_seat,u.source_parent},0,0});continue;}
    if(prompt.target.action==3) {
      if(pickup && simulation_.activate_admitted_control(prompt.target.object).attempted)
        u.source_player_actions.flags|=0x10u;
      begin_ready();continue;
    }
    if(pickup && prompt.target.action>=4 && prompt.target.action<=8 && prompt.target.action!=7) {simulation_.take_unit_action(u,prompt);continue;}
    if(prompt.target.action!=1 && prompt.target.action!=7){begin_ready();continue;}
    const auto transfer=native_take_weapon(u,content_,weapon_pickups_,prompt,dual_wield,tick,
        [this](Unit &unit,WeaponState &weapon){simulation_.clear_weapon_for_holster(unit,weapon);});
    if(transfer.attached || transfer.depleted)
      for(unsigned row=0;row<pickup_rows_.size();++row)
        if(pickup_rows_[row].object==transfer.object && !pickup_rows_[row].timer)
          pickup_rows_[row].timer=content_.netgame_equipment[row].respawn_seconds;
    if(transfer.depleted)simulation_.object_lifetimes_.release(transfer.object);
    begin_ready();
  }
  // C++ inventory actions above publish their new hands before firing/reload.
  // TU1.5 owns hand selection and cleanup in the upcoming native world phase;
  // step() reads those bindings after that completed phase. A native death
  // between frames may already detach FP while the CPP player still awaits
  // that readback, so this pre-world consumer must not publish native views.
  if(!source_object_world_||!source_object_world_->owns_physics()){
    update_first_person_views();update_replicated_first_person_views();
  }
  // The actual ready consumer above owns this action. Publish after rebuilding
  // the new hand's binding, so the event cannot target the old selected item.
  for(const auto &[unit,datum]:ready_animations)for(unsigned h=0;h<2;++h)
    if(const auto *weapon=native_hand_weapon(*unit,h);weapon && weapon->source_datum==datum)
      if(simulation_.first_person_activity_hook_)simulation_.first_person_activity_hook_(*unit,*weapon,0x05000024);
}
}
