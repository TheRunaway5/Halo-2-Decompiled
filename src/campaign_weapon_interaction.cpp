#include "halo2/native_unit_interaction.hpp"
#include "halo2/native_weapon_interaction.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/math.hpp"
#include <algorithm>
namespace halo2 {
void Simulation::equip_inventory(Unit &unit,std::uint32_t definition) {
  // Native campaign and match now retain the same single authoritative ammo
  // owner. This does not manufacture a source player or raw actor component.
  for(const auto handle:unit.source_inventory.slots)if(handle!=null_datum) {
    const auto placed=std::find_if(scene_.objects().begin(),scene_.objects().end(),[&](const auto &o){return o.handle==handle;});
    if(placed!=scene_.objects().end())scene_.destroy(handle);else object_lifetimes_.release(handle);
  }
  unit.source_inventory={};unit.weapon={};
  const auto handle=object_lifetimes_.reserve(2);
  if(handle==null_datum)throw FormatError("Campaign weapon object pool exhausted");
  object_lifetimes_.publish_native(handle,definition);
  unit.source_inventory.slots[0]=handle;
  const auto &d=content_.weapons.at(definition);
  auto equipped=native_weapon_state(d,handle);
  // Preserve the native campaign loadout's existing ammunition. Placed items
  // use their separate authored scenario counts below, never this equip path.
  equipped.loaded=std::max(0,d.capacity);equipped.reserve=std::max(0,d.reserve);
  equipped.smg.loaded=equipped.loaded;equipped.smg.reserve=equipped.reserve;
  equipped.shader.item_flags=5u|(unit.source_player!=null_datum?8u:0u);
  unit.source_inventory.inactive_weapons[0]=std::move(equipped);
  native_select_weapon(unit,0,std::uint32_t(ticks_));
}
void Simulation::synchronize_weapon_pickups() {
  if(content_.multiplayer_spawn)return;
  // A script-created item is born once per full salted generation. Carrying
  // it removes the world owner without destroying its script/object identity.
  for(const auto &object:scene_.objects()) {
    if(object.handle==null_datum)continue;
    const auto &placement=content_.objects.at(object.placement);
    if(placement.type!=2)continue;
    const auto definition=content_.weapons.find(placement.definition);
    if(definition==content_.weapons.end())continue;
    const auto &w=definition->second;
    if(w.first_person_model==null_datum || w.first_person_animation==null_datum)continue;
    auto item=std::find_if(campaign_weapon_pickups_.begin(),campaign_weapon_pickups_.end(),[&](const auto &p){return p.handle==object.handle;});
    if(item==campaign_weapon_pickups_.end()) {
      const auto basis=authored_basis(object.rotation);
      campaign_weapon_pickups_.push_back({object.handle,placement.definition,std::uint32_t(ticks_),
          object.position,basis.forward,basis.up,native_weapon_state(w,object.handle),object.visible,true,object.bsp});
      if(placement.weapon && w.capacity>0) {
        // FDA00 ->101980: the loaded count is clamped to the magazine;
        // unloaded uses total minus the nonnegative requested loaded count.
        auto &state=*campaign_weapon_pickups_.back().weapon;
        const int requested=std::max(0,int(placement.weapon->rounds_loaded));
        state.loaded=std::min(requested,w.capacity);
        state.reserve=std::max(0,int(placement.weapon->rounds_total)-requested);
        state.smg.loaded=state.loaded;state.smg.reserve=state.reserve;
      }
    } else {item->visible=object.visible;item->bsp=object.bsp;}

  }
  const auto retained=[&](std::uint32_t handle) {
    return std::any_of(scene_.objects().begin(),scene_.objects().end(),[&](const auto &o){return o.handle==handle;});
  };
  for(const auto &item:campaign_weapon_pickups_)if(item.scene_owned && !retained(item.handle)) {
    const auto discard=[&](Unit &u) {
      for(int slot=0;slot<4;++slot)if(u.source_inventory.slots[slot]==item.handle) {
        auto &i=u.source_inventory;i.slots[slot]=null_datum;i.inactive_weapons[slot].reset();
        for(unsigned hand=0;hand<2;++hand)if(i.current[hand]==slot)i.current[hand]=i.requested[hand]=-1;
        if(u.weapon.source_datum==item.handle)u.weapon={};
        native_inventory_changed(u);
      }
    };
    discard(player_);for(auto &u:actors_)discard(u);
  }
  std::erase_if(campaign_weapon_pickups_,[&](const auto &p){return p.scene_owned && !retained(p.handle);});
  const auto drop_dead=[&](Unit &u) {
    if(!u.exists || u.vitality.dead)
      for(int slot=0;slot<4;++slot)native_drop_weapon(u,slot,campaign_weapon_pickups_,std::uint32_t(ticks_));
  };
  drop_dead(player_);for(auto &u:actors_)drop_dead(u);
  for(const auto &item:campaign_weapon_pickups_)if(item.scene_owned && retained(item.handle)) {
    auto &retained_object=scene_.get(item.handle);
    retained_object.inventory_attached=std::any_of(player_.source_inventory.slots.begin(),player_.source_inventory.slots.end(),[&](auto h){return h==item.handle;});
    for(const auto &actor:actors_)retained_object.inventory_attached|=std::any_of(actor.source_inventory.slots.begin(),actor.source_inventory.slots.end(),[&](auto h){return h==item.handle;});
  }
  for(auto &item:campaign_weapon_pickups_)if(!item.scene_owned)
    item.visible=item.bsp<0 || std::size_t(item.bsp)==bsp_;
}
void Simulation::step_campaign_pickups(const Controls &input) {
  if(content_.multiplayer_spawn)return;
  auto &u=player_;auto &inventory=u.source_inventory;const auto tick=std::uint32_t(ticks_);
  const std::array<std::uint32_t,2> before{u.weapon.source_datum,
      native_hand_weapon(u,1)?native_hand_weapon(u,1)->source_datum:null_datum};
  auto actions=source_pickup_buttons(input.pickup,input.pickup?input.pickup_milliseconds:0,
      input.dual_wield,input.dual_wield?input.dual_wield_milliseconds:0,
      source_pickup_hold_threshold(content_.pickup_hold_seconds));
  std::uint32_t gameplay{};const bool consumed_dual=u.source_player_actions.flags&0x20u;
  source_player_action_edges(u.source_player_actions,gameplay,actions);
  if(u.source_action_word&0x400000u)return;
  const bool primary=actions&2u,dual=actions&0x20u;
  const auto prompt=action_prompt();
  const bool admitted_dual=dual && (prompt.target.action==1 || prompt.target.action==7) && (prompt.target.index&4);
  if(input.switch_weapon && !consumed_dual && !admitted_dual) {
    native_drop_secondary_weapon(u,campaign_weapon_pickups_,tick);
    for(int step=1;step<4;++step)if(native_select_weapon(u,(int(inventory.current[0])+step+4)%4,tick))break;
  }
  if(primary && u.source_parent!=null_datum) {
    take_unit_action(u,{{0,u.source_seat,u.source_parent},0,0});
  } else if(primary && prompt.target.action==3) {
    if(activate_admitted_control(prompt.target.object).attempted)u.source_player_actions.flags|=0x10u;
  } else if(primary && prompt.target.action>=4 && prompt.target.action<=8 && prompt.target.action!=7) {
    take_unit_action(u,prompt);
  } else if(primary && prompt.target.action==2) {
    for(auto &ally:actors_)if(ally.source_datum==prompt.target.object) {
      native_drop_secondary_weapon(u,campaign_weapon_pickups_,tick);
      native_trade_weapons(u,ally,content_,tick,allied(u.team,ally.team));break;
    }
  } else if(primary || dual) {
    const auto transfer=native_take_weapon(u,content_,campaign_weapon_pickups_,prompt,dual,tick);
    if(transfer.depleted) {
      const auto item=std::find_if(campaign_weapon_pickups_.begin(),campaign_weapon_pickups_.end(),[&](const auto &p){return p.handle==transfer.object;});
      if(item!=campaign_weapon_pickups_.end() && item->scene_owned)scene_.destroy(transfer.object);
      else object_lifetimes_.release(transfer.object);
    }
  }
  synchronize_weapon_pickups();
  for(unsigned hand=0;hand<2;++hand)if(auto *w=native_hand_weapon(u,hand);w && w->source_datum!=before[hand]) {
    const auto &d=content_.weapons.at(w->definition);
    w->ready_countdown=source_weapon_ready_begin(w->shader,d.ready_seconds,30);w->ready_clock=true;
    if(d.ready_effect!=null_datum && weapon_effect_hook_)
      weapon_effect_hook_(u,{d.ready_effect,w->source_datum,null_datum,0,0});
  }
}
}
