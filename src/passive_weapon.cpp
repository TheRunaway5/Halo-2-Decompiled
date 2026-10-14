#include "halo2/simulation.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/weapon_barrel_timing.hpp"
#include "halo2/weapon_energy.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
void Simulation::clear_weapon_for_holster(Unit &owner,WeaponState &state) {
  const auto found=content_.weapons.find(state.definition);
  if(found==content_.weapons.end() || !found->second.barrel_timing)return;
  const auto &w=found->second;
  if(w.smg) {
    state.magazine.mode=state.smg.magazine_mode;state.magazine.ticks=state.smg.magazine_ticks;
    state.magazine.transfer_ticks=state.smg.transfer_ticks;state.magazine.other_ticks=state.smg.magazine_other_ticks;
  }
  WeaponAmmunitionContext ammunition;ammunition.has_unit=true;
  ammunition.infinite=debug_settings_for(owner).unlimited_ammo;
  std::array<WeaponState *,5> owners{};owners[0]=&state;
  const auto view=[&](WeaponState &item) {
    const auto &definition=content_.weapons.at(item.definition);
    item.magazine.loaded=definition.smg?item.smg.loaded:std::bit_cast<std::int16_t>(std::uint16_t(item.loaded));
    item.magazine.reserve=definition.smg?item.smg.reserve:std::bit_cast<std::int16_t>(std::uint16_t(item.reserve));
    return WeaponAmmunitionView{item.source_datum,item.definition,{&item.magazine,1},
        definition.magazine?std::span(&*definition.magazine,1):std::span<const WeaponMagazineDefinition>{}};
  };
  ammunition.current=view(state);
  for(unsigned slot=0;slot<4;++slot)if(auto *other=native_inventory_weapon(owner,int(slot))) {
    owners[slot+1]=other;ammunition.inventory[slot]=view(*other);
  }
  const auto activity=[&](unsigned action) {
    if(action!=0)throw FormatError("Clear weapon reached an unsupported magazine activity");
    if(first_person_activity_hook_)first_person_activity_hook_(owner,state,0x0400000cu);
  };
  WeaponMagazineTransitionServices transition;transition.activity=activity;
  const auto cancel=[&] {
    //108230 resets trigger state/ticks and magazine modes; barrel state and
    //trigger flag words deliberately survive this owner.
    state.smg.trigger_state=state.smg.second_trigger_state=0;state.smg.trigger_ticks=0;
    if(w.magazine)weapon_magazine_transition(state.magazine,*w.magazine,0,30,transition);
  };
  //105750 runs before1042E0, with the weapon still in its actual unit slot.
  if(w.magazine && state.magazine.mode==1 &&
      int(state.magazine.transfer_ticks)*2<int(state.magazine.other_ticks)) {
    WeaponMagazineTransferServices transfer;
    transfer.available=[&]{return weapon_reserve_available(ammunition,0);};
    transfer.spend=[&](std::int32_t amount){(void)weapon_reserve_spend(ammunition,0,amount,[]{});};
    transfer.changed=[]{};
    weapon_magazine_transfer(state.magazine,*w.magazine,state.shader.item_flags,transfer);
    activity(0);weapon_magazine_transition(state.magazine,*w.magazine,5,30,transition);
  }
  cancel();
  //1042E0(immediate=true) forces recovery for every actual authored barrel.
  for(unsigned index=0;index<w.barrel_ammunition.size();++index) {
    auto &barrel=index==unsigned(w.barrel_timing->index)?state.smg.barrel:state.alternate_barrel;
    const auto &authored=w.barrel_ammunition[index];
    weapon_barrel_begin_recovery(barrel,authored.timing,30,w.barrel_timing->constants);
  }
  //The admitted native SMG own graph has no put-away action (checked on
  //import). Other native weapons have no represented own manager here.
  state.energy_blocking_ticks=first_person_timing_hook_?
      first_person_timing_hook_(owner,state,0x08000025u,1):0;
  //E9B06 and the second108230 precede FB3F0 and hand-selector removal.
  state.smg.input_flags=0;state.smg.held=false;cancel();
  for(auto *item:owners)if(item) {
    item->loaded=item->magazine.loaded;item->reserve=item->magazine.reserve;
    if(content_.weapons.at(item->definition).smg){item->smg.loaded=item->magazine.loaded;item->smg.reserve=item->magazine.reserve;}
  }
  state.smg.blocking_ticks=state.energy_blocking_ticks;
  state.smg.magazine_mode=state.magazine.mode;state.smg.magazine_ticks=state.magazine.ticks;
  state.smg.transfer_ticks=state.magazine.transfer_ticks;state.smg.magazine_other_ticks=state.magazine.other_ticks;
  state.reload=0;state.delay=float(state.smg.barrel.ticks)*tick_seconds;
}

void Simulation::update_passive_weapon(WeaponState &state,Unit *owner) {
  const auto found=content_.weapons.find(state.definition);
  if(found==content_.weapons.end() || !found->second.barrel_timing)return;
  const auto &w=found->second;auto &barrel=state.smg.barrel;
  if(w.smg) {
    state.energy_blocking_ticks=state.smg.blocking_ticks;
    state.magazine.mode=state.smg.magazine_mode;state.magazine.ticks=state.smg.magazine_ticks;
    state.magazine.transfer_ticks=state.smg.transfer_ticks;state.magazine.other_ticks=state.smg.magazine_other_ticks;
  }
  if((barrel.flags&2u) || (state.shader.weapon_flags&1u))
    throw FormatError("Passive prepared projectile needs its original postpass owner");
  if(w.smg && state.smg.own_manager && !(state.smg.own_events&1u) &&
      ((state.smg.own_flags&2u) || w.smg->single_own_frames==1) &&
      (state.smg.weapon_state<7 || (state.smg.weapon_state>8 && state.smg.weapon_state!=10)))
    state.smg.weapon_state=0;
  source_weapon_ready_tick(state.shader,state.ready_countdown);
  const auto effect=[&](std::uint32_t tag) {
    const WeaponEffectInput input{tag,state.source_datum,null_datum,0,0};
    if(owner && weapon_effect_hook_)return weapon_effect_hook_(*owner,input);
    if(!owner && world_weapon_effect_hook_)return world_weapon_effect_hook_(input);
    return null_datum; // Explicit unavailable generic allocation, as in the active adapter.
  };
  const auto activity=[&](std::uint32_t symbol) {
    if(owner && first_person_activity_hook_)first_person_activity_hook_(*owner,state,symbol);
  };
  const auto timing=[&](std::uint32_t symbol,unsigned section) {
    return owner && first_person_timing_hook_?first_person_timing_hook_(*owner,state,symbol,std::int16_t(section)):std::int16_t(0);
  };
  if(w.energy) {
    const auto &d=*w.energy;
    WeaponEnergyState source{state.definition,state.shader.item_flags,state.shader.weapon_flags,
        state.shader.heat,state.shader.power_fraction,state.cooling_inhibitor,state.overheat_handle,state.energy_blocking_ticks};
    WeaponHeatServices services;services.has_unit=owner!=nullptr;
    services.reset_zoom=[&]{source_zoom_reset(owner->source_zoom);++owner->zoom_reset_serial;};
    services.action=[&](unsigned action){activity(action==14?0x0b00006du:0x08000071u);};
    services.vent_timing_ticks=[&]{return timing(0x08000071u,1);};
    weapon_heat_step(source,d.heat,tick_seconds,d.one,services);
    state.shader.weapon_flags=source.weapon_flags;state.shader.heat=source.heat;
    state.cooling_inhibitor=source.cooling_inhibitor;state.energy_blocking_ticks=source.blocking_ticks;
    state.overheat_handle=source.overheat_handle;
  } else if(state.energy_blocking_ticks>0)--state.energy_blocking_ticks;
  WeaponAmmunitionContext ammunition;ammunition.has_unit=owner!=nullptr;
  ammunition.infinite=owner && debug_settings_for(*owner).unlimited_ammo;
  std::array<WeaponState *,5> owners{};owners[0]=&state;
  const auto view=[&](WeaponState &item) {
    const auto &definition=content_.weapons.at(item.definition);
    item.magazine.loaded=definition.smg?item.smg.loaded:std::bit_cast<std::int16_t>(std::uint16_t(item.loaded));
    item.magazine.reserve=definition.smg?item.smg.reserve:std::bit_cast<std::int16_t>(std::uint16_t(item.reserve));
    return WeaponAmmunitionView{item.source_datum,item.definition,{&item.magazine,1},
        definition.magazine?std::span(&*definition.magazine,1):std::span<const WeaponMagazineDefinition>{}};
  };
  ammunition.current=view(state);
  if(owner)for(unsigned slot=0;slot<4;++slot)if(auto *other=native_inventory_weapon(*owner,int(slot))) {
    owners[slot+1]=other;ammunition.inventory[slot]=view(*other);
  }
  const auto symbol=[](unsigned action)->std::uint32_t {
    switch(action) {
      case 0:return 0x0400000cu;case 7:return 0x0c000064u;case 8:return 0x0b000065u;
      case 9:return 0x0c0005aeu;case 10:return 0x150005afu;case 11:return 0x140005b0u;
      case 12:return 0x0b0005b1u;default:throw FormatError("Passive magazine activity exceeds source table");
    }
  };
  if(w.magazine && !(w.inventory_flags&((1u<<11)|(1u<<19)|(1u<<25)))) {
    WeaponMagazineServices services;
    services.transfer.available=[&]{return weapon_reserve_available(ammunition,0);};
    services.transfer.spend=[&](std::int32_t amount){(void)weapon_reserve_spend(ammunition,0,amount,[]{});};
    services.transfer.changed=[]{};
    services.transition.activity_symbol=symbol;services.transition.timing=timing;
    services.transition.activity=[&](unsigned action){activity(symbol(action));};
    services.idle=[&]{return !state.smg.trigger_state && !state.smg.second_trigger_state && !state.smg.weapon_state;};
    services.request_state=[&](unsigned requested) {
      if(state.smg.own_manager && (!state.smg.weapon_state ||
          (state.smg.weapon_state<3 && state.smg.weapon_state<=requested)))state.smg.weapon_state=requested;
    };
    services.effect=[&](std::uint32_t tag){(void)effect(tag);};
    //101D50 requires this exact datum in a current hand before its unit action.
    //These objects have no current hand; a retained reload still advances.
    services.request_reload=[]{return false;};
    services.fire_admitted=[](unsigned){return false;}; // Declared zero passive input.
    weapon_magazine_step(state.magazine,*w.magazine,state.shader.item_flags,0,0,30,services);
    for(auto *item:owners)if(item) {
      item->loaded=item->magazine.loaded;item->reserve=item->magazine.reserve;
      if(content_.weapons.at(item->definition).smg){item->smg.loaded=item->magazine.loaded;item->smg.reserve=item->magazine.reserve;}
    }
  }
  state.smg.input_flags=0;state.smg.held=false;
  //1028C7/10295C/1029D5/102A74 with the declared idle trigger and zero
  //passive input. Latching styles arm bit0 and preserve bit5 on release.
  if(state.smg.trigger_state)throw FormatError("Passive non-idle trigger requires its original update owner");
  if(w.trigger_behavior==0)state.smg.trigger_flags&=~32u;
  else if(w.trigger_behavior==1 || w.trigger_behavior==4 || w.trigger_behavior==5)state.smg.trigger_flags|=1u;
  for(unsigned index=0;index<w.barrel_ammunition.size();++index) {
    auto &active=index==unsigned(w.barrel_timing->index)?barrel:state.alternate_barrel;
    const auto &authored=w.barrel_ammunition[index];
    if(active.mode!=1)active.flags&=~0x40u;
    if(active.effect_b>0.f)active.effect_b=std::max(0.f,active.effect_b-authored.effect_decay*tick_seconds);
    WeaponBarrelTimingContext context;context.tick_rate=30;context.spent=state.shader.power_fraction;
    if(w.barrel_ammunition.size()==2)context.alternate_mode=index==unsigned(w.barrel_timing->index)?state.alternate_barrel.mode:barrel.mode;
    const bool blocked=state.energy_blocking_ticks>0 || (state.shader.weapon_flags&2u);
    const auto next=weapon_barrel_timing_step(active,authored.timing,context,w.barrel_timing->constants,blocked);
    if(next.prepare)throw FormatError("Passive firing admission requires its original preparation/projectile owner");
    if(next.marker)active.rate=std::min(1.f,active.rate+authored.rate_rise*tick_seconds);
    else if(active.rate>0.f)active.rate=std::max(0.f,active.rate-authored.rate_fall*tick_seconds);
    WeaponErrorState error{active.error,active.angle_change};
    //D2940 returns literal one for an ownerless weapon. The declared
    //retained unit reads its current shared opposite-hand feedback.
    weapon_error_step(error,authored.error,tick_seconds,owner?weapon_hand_blend(*owner,state):w.barrel_timing->constants.one,next.marker);
    active.error=error.error;active.angle_change=error.recoil;
  }
  state.smg.weapon_flags=state.shader.weapon_flags;state.smg.blocking_ticks=state.energy_blocking_ticks;
  if(w.magazine) {
    state.smg.magazine_mode=state.magazine.mode;state.smg.magazine_ticks=state.magazine.ticks;
    state.smg.transfer_ticks=state.magazine.transfer_ticks;state.smg.magazine_other_ticks=state.magazine.other_ticks;
  }
  state.reload=state.magazine.mode==1?float(state.magazine.ticks)*tick_seconds:0.f;
  state.delay=float(barrel.ticks)*tick_seconds;
}
} // namespace halo2
