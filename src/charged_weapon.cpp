#include "halo2/simulation.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <bit>
#include <cmath>
namespace halo2 {
namespace {
std::uint8_t accumulated_charge(std::uint8_t value,float rate,float scale) {
  const float previous=float(value)*std::bit_cast<float>(0x3b808081u);
  float step=rate*std::bit_cast<float>(0x3e124925u); // Original446424/449EB0.
  if(scale>0.f)step=step*scale;
  volatile double sum=double(step)+double(previous);
  volatile double scaled=sum*255.0;
  const auto integer=scaled>=-9223372036854775808.0 && scaled<9223372036854775808.0?
      std::int64_t(scaled):(-9223372036854775807LL-1);
  return std::uint8_t(std::uint64_t(integer));
}
unsigned firing_entry(SmgBarrelState &barrel,const WeaponDefinition &definition) {
  const auto count=definition.firing_effects.size();
  if(!count || count>16)throw FormatError("Charged preparation requires its actual effect bank");
  if(barrel.effect_remaining<1) {
    unsigned index=std::uint16_t(barrel.effect_index);
    do {
      if(barrel.effect_mask==((1u<<count)-1))barrel.effect_mask=0;
      do {index=(index+1)%count;}while(barrel.effect_mask&(1u<<index));
      barrel.effect_index=std::int16_t(index);barrel.effect_mask|=std::uint16_t(1u<<index);
      const auto &row=definition.firing_effects[index];
      if(row.minimum!=row.maximum || row.minimum<1 || definition.random_firing_effects)
        throw FormatError("Charged preparation requires its recovered fixed repeat contract");
      barrel.effect_remaining=row.minimum;
    }while(barrel.effect_remaining<1);
  }
  --barrel.effect_remaining;return std::uint16_t(barrel.effect_index);
}
}
void Simulation::update_charged_weapon(Unit &unit,WeaponState &state,const WeaponDefinition &definition,
    bool fire,bool blocked,unsigned hand,int owner) {
  if(definition.barrel_views.size()!=2 || definition.triggers.size()!=1 || !definition.energy)
    throw FormatError("Charged weapon lost its original two-barrel definition");
  const auto &energy=*definition.energy;
  const bool infinite=debug_settings_for(unit).unlimited_ammo;
  const float feedback=weapon_hand_blend(unit,state);
  const bool rising=fire && !state.smg.held;state.smg.held=fire;
  state.smg.input_flags=fire?std::uint16_t(2u|(rising?8u:0u)):0;
  state.trigger_input.current=fire?255:0; //1045E0/105500 with literal digital fraction1/0.
  state.shader.weapon_flags&=~1u;
  WeaponEnergyState source{state.definition,state.shader.item_flags,state.shader.weapon_flags,
      state.shader.heat,state.shader.power_fraction,state.cooling_inhibitor,state.overheat_handle,state.energy_blocking_ticks};
  const auto publish_heat=[&] {
    state.shader.weapon_flags=source.weapon_flags;state.shader.heat=source.heat;
    state.cooling_inhibitor=source.cooling_inhibitor;state.overheat_handle=source.overheat_handle;
    state.energy_blocking_ticks=source.blocking_ticks;
  };
  WeaponHeatServices heat;heat.has_unit=(state.shader.item_flags&1u) && unit.source_datum!=null_datum;
  heat.reset_zoom=[&]{publish_heat();source_zoom_reset(unit.source_zoom);++unit.zoom_reset_serial;};
  heat.action=[&](unsigned action){publish_heat();weapon_activity(unit,state,action==14?0x0b00006du:0x08000071u);};
  heat.vent_timing_ticks=[&]{publish_heat();return first_person_timing_hook_?
      first_person_timing_hook_(unit,state,0x08000071u,1):std::int16_t(0);};
  weapon_heat_step(source,energy.heat,tick_seconds,energy.one,heat);
  publish_heat();
  blocked=blocked || state.energy_blocking_ticks>0 || (state.shader.weapon_flags&2u);
  if(blocked)state.trigger_input.ticks=0; //1027C5 precedes the trigger loop.
  std::array<SmgBarrelState,2> barrels{state.smg.barrel,state.alternate_barrel};
  WeaponTriggerState trigger{state.smg.trigger_state,0,state.smg.trigger_ticks,state.smg.trigger_flags,state.smg.trigger_effect_handle};
  const auto publish=[&] {
    state.smg.barrel=barrels[0];state.alternate_barrel=barrels[1];
    state.smg.trigger_state=trigger.mode;state.smg.trigger_ticks=trigger.ticks;
    state.smg.trigger_flags=trigger.flags;state.smg.trigger_effect_handle=trigger.effect;
  };
  WeaponTriggerPhaseContext context;context.tick_rate=30;context.blocked=blocked;context.held=fire;context.pressed=rising;
  context.has_unit=heat.has_unit;context.has_player=unit.source_player!=null_datum;
  context.spent=state.shader.power_fraction;context.barrel_count=2;
  WeaponTriggerPhaseServices services;
  services.allocate_effect=[&](std::uint32_t tag) {
    publish();
    return weapon_effect_hook_?weapon_effect_hook_(unit,{tag,state.source_datum,null_datum,0,0}):null_datum;
  };
  services.reset_zoom=[&]{publish();source_zoom_reset(unit.source_zoom);++unit.zoom_reset_serial;};
  services.weapon_state=[&](unsigned requested,bool force) {
    publish();
    if(state.smg.own_manager && (force || !state.smg.weapon_state ||
        (state.smg.weapon_state<3 && state.smg.weapon_state<=requested)))state.smg.weapon_state=requested;
  };
  services.activity=[&](unsigned action) {
    publish();
    if(action!=13)throw FormatError("Charged trigger reached an additional original unit action owner");
    weapon_activity(unit,state,0x0b000067u);
  };
  services.stop_effect=[&](std::uint32_t) {
    throw FormatError("Charged effect deactivation requires its actual original effect lifetime owner");
  };
  services.barrel_admitted=[&](std::int16_t index) {
    if(index<0 || index>=2)return false;
    const auto &barrel=barrels[unsigned(index)];
    if(barrel.mode==2 || (barrel.flags&4u) || (state.shader.weapon_flags&2u))return false;
    if((definition.inventory_flags&(1u<<11)) && state.shader.power_fraction>=1.f && (barrel.flags&0x40u) && !infinite)return false;
    return true;
  };
  (void)weapon_trigger_step(trigger,definition.triggers[0],barrels,state.trigger_input,state.trigger_target,0,context,services);
  publish();
  for(unsigned index=0;index<2;++index) {
    auto &barrel=barrels[index];const auto &view=*definition.barrel_views[index];const auto &d=*view.barrel_timing;
    if(barrel.effect_a>0.f){barrel.effect_a-=view.barrel_effect_a_decay*tick_seconds;if(barrel.effect_a<=0.f)barrel.effect_a=0;}
    if(barrel.effect_b>0.f){barrel.effect_b-=d.effect_decay*tick_seconds;if(barrel.effect_b<=0.f)barrel.effect_b=0;}
    WeaponBarrelTimingContext timing;timing.tick_rate=30;timing.spent=state.shader.power_fraction;
    timing.alternate_mode=barrels[index?0:1].mode;
    if(const auto *other=native_hand_weapon(unit,hand?0:1);other && other->definition==state.definition) {
      const auto &other_barrel=index?other->alternate_barrel:other->smg.barrel;
      timing.other_hand=WeaponBarrelTimingContext::OtherHand{other_barrel.mode,other_barrel.counter};
    }
    const auto next=weapon_barrel_timing_step(barrel,d.definition,timing,d.constants,blocked);
    if(next.prepare) {
      const bool admitted=!(definition.inventory_flags&(1u<<11)) || state.shader.power_fraction<1.f;
      const auto selected=firing_entry(barrel,view);const auto &effect=view.firing_effects[selected];
      const bool misfire=weapon_battery_misfire(state.shader.power_fraction,energy.misfire_threshold,
          energy.misfire_chance,energy.one,energy.random_scale,random_state_);
      const unsigned selection=!admitted?2u:misfire?1u:0u;
      const float a=admitted?barrel.rate:1.f;
      const float b=admitted && !misfire?state.shader.heat/energy.heat.maximum:0.f;
      if(admitted) {
        barrel.subshot_count=0;++barrel.shot_count;++barrel.marker_count;state.last_shot=ticks_+1;
        publish();
        weapon_activity(unit,state,misfire?(index?0x0900006bu:0x0900006au):(index?0x06000007u:0x06000006u));
        if(view.barrel_effect_a_threshold>0.f && !(d.definition.flags&16u))barrel.effect_a=1.f;
        if(view.barrel_effect_b_threshold>0.f)barrel.effect_b=1.f;
        if(unit.source_player!=null_datum)disrupt_camouflage(unit,definition.firing_camouflage_amount,definition.firing_camouflage_limit);
        state.shader.heat=state.shader.heat+view.energy->heat_per_shot;
        const float maximum=(state.shader.item_flags&8u)?energy.one:energy.heat.maximum;
        if(state.shader.heat>maximum)state.shader.heat=maximum;
        if(!(definition.inventory_flags&(1u<<26))) {
          WeaponEnergyState age{state.definition,state.shader.item_flags,state.shader.weapon_flags,state.shader.heat,state.shader.power_fraction};
          weapon_age(age,view.energy->spent_per_shot,energy.one,infinite,energy.age,[&](std::uint32_t tag){
            state.shader.power_fraction=age.spent;(void)services.allocate_effect(tag);
          });
          state.definition=age.definition;state.shader.power_fraction=age.spent;
        }
        if(!misfire){barrel.flags|=2u;state.shader.weapon_flags|=1u;}
        if(definition.weapon_type==3 && index==1)state.shader.weapon_flags|=8u;
      } else barrel.flags|=0xc0u;
      if(effect.damage_effects[selection]!=null_datum && rumble_events_.size()<256)
        rumble_events_.push_back({effect.damage_effects[selection],unit.vitality.shield>0,1.f,owner});
      if(weapon_heat_detonation(state.shader.heat,energy.detonation_threshold,energy.detonation_chance,energy.random_scale,random_state_))
        throw FormatError("Charged self-detonation requires its original object lifetime owner");
      barrel.counter=0;
      publish();
      if(weapon_effect_hook_)barrel.effect_handle=weapon_effect_hook_(unit,{effect.effects[selection],state.source_datum,
          barrel.effect_handle,std::bit_cast<std::uint32_t>(a),std::bit_cast<std::uint32_t>(b)});
    }
    if(next.marker){barrel.rate=d.rate_rise*tick_seconds+barrel.rate;if(barrel.rate>1.f)barrel.rate=1.f;}
    else if(barrel.rate>0.f){barrel.rate=barrel.rate-d.rate_fall*tick_seconds;if(barrel.rate<0.f)barrel.rate=0.f;}
    if(barrel.rate>0.f)state.charge_fraction=accumulated_charge(state.charge_fraction,barrel.rate,view.barrel_charge_scale);
    WeaponErrorState error{barrel.error,barrel.angle_change};weapon_error_step(error,view.firing_error,tick_seconds,feedback,next.marker);
    barrel.error=error.error;barrel.angle_change=error.recoil;
  }
  publish();
  state.smg.weapon_flags=std::uint16_t(state.shader.weapon_flags);state.smg.blocking_ticks=state.energy_blocking_ticks;
  state.delay=float(barrels[0].ticks)*tick_seconds;
}
}
