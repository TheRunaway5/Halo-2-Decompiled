#include "halo2/weapon_trigger.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace halo2 {
namespace {
std::int16_t duration(float seconds,std::int16_t rate) {
  const float rounded=std::nearbyint(float(rate)*seconds);
  const auto word=rounded>=-2147483648.f && rounded<2147483648.f?
      std::int32_t(rounded):std::int32_t(-2147483647-1);
  return std::bit_cast<std::int16_t>(std::uint16_t(std::uint32_t(word)));
}
unsigned threshold(float value) {
  //106960 uses x87 FMUL255 then the real CRT truncating64-bit conversion.
  const double product=double(value)*255.0;
  const auto converted=product>=-9223372036854775808.0 && product<9223372036854775808.0?
      std::int64_t(product):(-9223372036854775807LL-1);
  return std::uint8_t(std::uint64_t(converted));
}
void zoom(bool has_unit,const WeaponTriggerServices &services) {
  if(!has_unit)return;
  if(!services.reset_zoom)throw std::runtime_error("Original trigger zoom owner unavailable");
  services.reset_zoom();
}
}
WeaponTriggerInputEvents weapon_trigger_input(WeaponTriggerInputState &input,
    const WeaponTriggerState &state,const WeaponTriggerDefinition &definition,std::int16_t rate) {
  WeaponTriggerInputEvents out;
  const float rounded=std::nearbyint(float(rate)*definition.autofire_seconds);
  const auto wait=rounded>=-2147483648.f && rounded<2147483648.f?
      std::int32_t(rounded):std::int32_t(-2147483647-1);
  const unsigned delta_limit=threshold(definition.autofire_threshold);
  if(input.current<input.previous) {
    if(input.direction==1) {
      out.primary=int(input.previous)-int(input.anchor)>20;
      input.direction=255;input.anchor=input.previous;input.ticks=0;
    }
  } else {
    bool advance=input.current==input.previous && input.direction==1;
    if(input.current>input.previous) {
      if(input.direction==255) {
        out.released=int(input.anchor)-int(input.previous)>20;
        input.anchor=input.previous;input.direction=1;input.ticks=0;
      } else if(input.direction==0) {
        input.anchor=0;input.direction=1;input.ticks=0;
      } else advance=true;
    }
    if(advance) {
      const int next=std::bit_cast<std::int8_t>(input.ticks)+1;
      input.ticks=std::uint8_t(next>255?255:next);
      if(std::bit_cast<std::int8_t>(input.ticks)>=wait) {
        if(int(input.current)-int(input.anchor)<int(delta_limit) && input.current<250)out.primary=true;
        else out.charged=true;
      }
    }
  }
  if(!input.current && !out.primary && !out.charged)out.released=true;
  else if(!out.released) {
    if(out.primary && (state.flags&0x18u))out.primary=false;
    if(out.charged && ((state.flags&0x10u) || ((definition.flags&1u) && (state.flags&8u))))out.charged=false;
  }
  input.previous=input.current;input.current=0;
  return out;
}
bool weapon_trigger_charge_admitted(const WeaponTriggerDefinition &definition,
    std::span<const SmgBarrelState> barrels,float spent) {
  const auto index=definition.secondary_barrel;
  if(index!=-1 && index<int(barrels.size())) {
    if(index<0)throw std::runtime_error("Original trigger references invalid negative barrel storage");
    if(barrels[unsigned(index)].mode)return false;
  }
  return !(spent>=1.f);
}
void weapon_trigger_start_charge(WeaponTriggerState &state,const WeaponTriggerDefinition &definition,
    std::int16_t rate,bool has_unit,const WeaponTriggerServices &services) {
  if(!services.allocate_effect)throw std::runtime_error("Original trigger effect owner unavailable");
  state.effect=services.allocate_effect(definition.charging_effect);
  state.mode=1;state.ticks=duration(definition.charging_seconds,rate);zoom(has_unit,services);
}
void weapon_trigger_finish_charge(WeaponTriggerState &state,const WeaponTriggerDefinition &definition,
    std::int16_t rate,unsigned index,const WeaponTriggerServices &services) {
  if(!services.weapon_state || !services.activity)throw std::runtime_error("Original charged trigger action owner unavailable");
  state.mode=2;state.ticks=duration(definition.charged_seconds,rate);
  services.weapon_state(index+7,true);services.activity(13);
}
void weapon_trigger_release_charge(WeaponTriggerState &state,const WeaponTriggerDefinition &definition,
    std::span<SmgBarrelState> barrels,std::int16_t rate) {
  if(definition.discharging_seconds>0.f) {
    state.mode=4;state.ticks=duration(definition.discharging_seconds,rate);return;
  }
  if(definition.secondary_barrel!=-1) {
    if(definition.secondary_barrel<0 || unsigned(definition.secondary_barrel)>=barrels.size())
      throw std::runtime_error("Original release trigger has no secondary barrel storage");
    barrels[unsigned(definition.secondary_barrel)].flags|=1u;
  }
  state.mode=0;state.ticks=0;state.flags&=~0x18u;
}
void weapon_trigger_lock(WeaponTriggerState &state,WeaponTriggerLockPhase phase,
    std::int16_t rate,bool has_unit,const WeaponTriggerServices &services) {
  switch(phase) {
    case WeaponTriggerLockPhase::Seeking:state.mode=3;state.ticks=0;break;
    case WeaponTriggerLockPhase::Lost:state.mode=7;state.ticks=0;break;
    case WeaponTriggerLockPhase::Acquiring:state.mode=5;state.ticks=duration(.5f,rate);break;
    case WeaponTriggerLockPhase::Locked:state.mode=6;state.ticks=rate;break;
  }
  zoom(has_unit,services);
}
void weapon_trigger_clear_barrel(std::span<SmgBarrelState> barrels,std::int16_t index) {
  if(index<0 || index>=2)return;
  if(unsigned(index)>=barrels.size())throw std::runtime_error("Original clear trigger has no barrel storage");
  if(barrels[unsigned(index)].mode!=1)barrels[unsigned(index)].flags&=~0x40u;
}
}
namespace halo2 {
bool weapon_trigger_step(WeaponTriggerState &state,const WeaponTriggerDefinition &definition,
    std::span<SmgBarrelState> barrels,WeaponTriggerInputState &analog,WeaponTriggerTarget &target,
    unsigned index,const WeaponTriggerPhaseContext &context,const WeaponTriggerPhaseServices &services) {
  bool dirty=state.ticks>0;if(dirty)--state.ticks;
  const bool held=!context.blocked && (context.held || (state.flags&0x80u));
  const bool pressed=!context.blocked && context.pressed;
  auto admitted=[&](std::int16_t barrel) {
    if(!services.barrel_admitted)throw std::runtime_error("Original trigger barrel admission owner unavailable");
    return services.barrel_admitted(barrel);
  };
  auto fire=[&](std::int16_t barrel,bool consume_latch=false) {
    if(barrel==-1 || !admitted(barrel))return;
    if(barrel<0 || unsigned(barrel)>=barrels.size())throw std::runtime_error("Original trigger fire has no barrel storage");
    barrels[unsigned(barrel)].flags|=1u;if(consume_latch)state.flags&=~1u;
  };
  auto clear=[&]{weapon_trigger_clear_barrel(barrels,definition.primary_barrel);
      weapon_trigger_clear_barrel(barrels,definition.secondary_barrel);};
  auto charge_ok=[&]{
    if(context.barrel_count>barrels.size())throw std::runtime_error("Original trigger barrel count exceeds actual storage");
    return weapon_trigger_charge_admitted(definition,barrels.first(context.barrel_count),context.spent);
  };
  auto charge=[&]{weapon_trigger_start_charge(state,definition,context.tick_rate,context.has_unit,services);};
  auto lock=[&](WeaponTriggerLockPhase phase){weapon_trigger_lock(state,phase,context.tick_rate,context.has_unit,services);};
  auto acquire=[&]() {
    if(!services.acquire_target)throw std::runtime_error("Original trigger target acquisition owner unavailable");
    return services.acquire_target();
  };
  auto stop=[&] {
    if(state.effect==0xffffffffu)return;
    if(!services.stop_effect)throw std::runtime_error("Original trigger effect lifetime owner unavailable");
    services.stop_effect(state.effect);state.effect=0xffffffffu;
  };
  auto release_state=[&]{state.mode=0;state.ticks=0;};
  switch(state.mode) {
    case 0:
      switch(definition.behavior) {
        case 0:
          if((held || pressed) && definition.primary_barrel!=-1 && admitted(definition.primary_barrel)) {
            state.flags|=0x20u;
            if(definition.primary_barrel<0 || unsigned(definition.primary_barrel)>=barrels.size())
              throw std::runtime_error("Original automatic trigger has no primary barrel storage");
            barrels[unsigned(definition.primary_barrel)].flags|=1u;
          } else state.flags&=~0x20u;
          if(!held || pressed)weapon_trigger_clear_barrel(barrels,definition.primary_barrel);
          break;
        case 1:
          if(!held){state.flags|=1u;weapon_trigger_clear_barrel(barrels,definition.primary_barrel);}
          else if(state.flags&1u)fire(definition.primary_barrel,true);
          break;
        case 2:
          if(context.blocked){analog.previous=analog.current=analog.direction=0;break;}
          if(!analog.current && !analog.previous) {
            if(!pressed){state.flags&=~0x18u;clear();}
            else if(definition.charging_type==1 && charge_ok()){charge();state.flags|=0x40u;}
          } else {
            const auto input=weapon_trigger_input(analog,state,definition,context.tick_rate);
            if(input.primary) {
              state.flags|=8u;
              switch(definition.discharge_type) {
                case 0:fire(definition.primary_barrel);break;
                case 1:if(charge_ok())charge();break;
                case 2:lock(WeaponTriggerLockPhase::Seeking);break;
                case 3:fire(definition.secondary_barrel);break;
              }
            }
            if(input.charged) {
              state.flags|=0x10u;
              switch(definition.charging_type) {
                case 0:fire(definition.secondary_barrel);break;
                case 1:if(charge_ok())charge();break;
                case 2:lock(context.has_player?WeaponTriggerLockPhase::Seeking:WeaponTriggerLockPhase::Locked);break;
                case 3:fire(definition.primary_barrel);break;
              }
            }
            if(input.released){state.flags&=~0x18u;clear();}
          }
          break;
        case 3:
          if(!(held || pressed) || !charge_ok())weapon_trigger_clear_barrel(barrels,definition.primary_barrel);
          else {if(pressed)state.flags|=0x40u;charge();}
          break;
        case 4:
          if(!held){state.flags|=1u;clear();}
          else if(state.flags&1u)fire(context.alternate?definition.secondary_barrel:definition.primary_barrel,true);
          break;
        case 5:
          if(!held){state.flags|=1u;clear();}
          else if((state.flags&1u) && admitted(0)) {
            if(!context.has_player)fire(definition.secondary_barrel,true);
            else if(acquire())lock(WeaponTriggerLockPhase::Acquiring);
            else fire(definition.primary_barrel,true);
          }
          break;
      }
      break;
    case 1:
      if(!state.ticks)weapon_trigger_finish_charge(state,definition,context.tick_rate,index,services);
      else if(!held && !pressed) {
        if(!(state.flags&0x40u)) {
          const auto barrel=definition.primary_barrel;
          if(barrel!=-1) {
            if(barrel<0 || unsigned(barrel)>=barrels.size())throw std::runtime_error("Original early charge release has no barrel storage");
            barrels[unsigned(barrel)].flags|=1u;
          }
        } else state.flags&=~0x40u;
        release_state();stop();
      }
      break;
    case 2:
      if(!(state.flags&0x40u)) {
        if(!held && !context.blocked)weapon_trigger_release_charge(state,definition,barrels,context.tick_rate);
        if(!state.ticks) {
          if(definition.charged_action==1 && !context.editor) {
            if(!services.allocate_effect || !services.destroy_weapon)throw std::runtime_error("Original charged weapon destruction owner unavailable");
            (void)services.allocate_effect(context.detonation_effect);services.destroy_weapon();
          } else if(definition.charged_action==2)weapon_trigger_release_charge(state,definition,barrels,context.tick_rate);
        }
      } else if(!pressed){release_state();stop();state.flags&=~0x40u;}
      break;
    case 3:case 7:
      if(!held && !context.blocked) {
        if(state.mode!=7)fire(definition.primary_barrel,true);
        release_state();
      } else if(acquire())lock(WeaponTriggerLockPhase::Acquiring);
      break;
    case 5:
      if(!held && !context.blocked){fire(definition.primary_barrel,true);release_state();}
      else if(const auto acquired=acquire()) {
        if(acquired->datum==target.datum){target.marker=acquired->marker;if(!state.ticks)lock(WeaponTriggerLockPhase::Locked);}
        else {lock(WeaponTriggerLockPhase::Acquiring);target=*acquired;}
      } else lock(WeaponTriggerLockPhase::Lost);
      break;
    case 6:
      if(!held && !context.blocked){fire(definition.secondary_barrel);release_state();}
      else {
        if(!services.track_target)throw std::runtime_error("Original trigger tracking owner unavailable");
        if(const auto tracked=services.track_target();tracked && tracked->datum==target.datum)lock(WeaponTriggerLockPhase::Locked);
        if(!state.ticks && context.has_player)lock(WeaponTriggerLockPhase::Lost);
      }
      break;
  }
  return dirty || state.mode!=0 || held || pressed;
}
}
