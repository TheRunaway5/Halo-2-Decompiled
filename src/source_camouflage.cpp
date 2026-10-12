#include "halo2/source_camouflage.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr auto none=std::uint32_t(-1);
SourceCamouflagePlayerView player(std::uint32_t datum,const SourceCamouflageServices& services) {
  if(!services.player)throw std::runtime_error("Source camouflage player owner unavailable");
  return services.player(datum);
}
SourceCamouflageUnitView unit(std::uint32_t datum,const SourceCamouflageServices& services) {
  if(!services.unit)throw std::runtime_error("Source camouflage unit owner unavailable");
  return services.unit(datum);
}
void dirty(const SourceCamouflageUnitView& state,const SourceCamouflageServices& services) {
  if(state.replication_entity==none)return;
  if(!services.dirty)throw std::runtime_error("Source camouflage replication owner unavailable");
  services.dirty(state.replication_entity,0x800000u);
}
std::int16_t half_rate(std::int16_t rate,std::uint16_t control_word) {
  // Every signed16 rate times 0.5 is exactly representable in binary32. Model
  // FISTP's RC bits explicitly, independently of the host rounding environment.
  const float value=float(rate)*0.5f;
  float rounded;
  switch((control_word>>10)&3) {
    case 1:rounded=std::floor(value);break;
    case 2:rounded=std::ceil(value);break;
    case 3:rounded=std::trunc(value);break;
    default: {
      const auto lower=std::floor(value);
      const auto fraction=value-lower;
      rounded=fraction<0.5f ? lower : fraction>0.5f ? lower+1.f :
          (std::int32_t(lower)&1) ? lower+1.f : lower;
      break;
    }
  }
  return std::bit_cast<std::int16_t>(std::uint16_t(std::int32_t(rounded)));
}
void activate(std::uint32_t datum,std::int16_t index,const SourceCamouflageServices& services) {
  if(index!=0)return;
  auto state=unit(player(datum,services).current_unit,services);
  state.flags|=8u;state.fade_speed=0.25f;dirty(state,services);
}
}
void source_powerup_grant(std::uint32_t datum,std::int16_t index,std::int16_t ticks,
    const SourceCamouflageServices& services) {
  if(index<0 || index>=2)throw std::runtime_error("Source powerup index exceeds the two admitted timers");
  auto& timer=player(datum,services).timers[unsigned(index)];
  if(timer==0)activate(datum,index,services);
  if(timer<=ticks)timer=ticks;
}
void source_camouflage_refresh(bool engine,std::uint32_t datum,std::int16_t rate,
    std::uint16_t control_word,const SourceCamouflageServices& services) {
  if(!engine)return;
  if(!services.trait)throw std::runtime_error("Source camouflage trait owner unavailable");
  if(!services.trait(datum,1))return;
  if(player(datum,services).current_unit==none)return;
  source_powerup_grant(datum,0,half_rate(rate,control_word),services);
}
void source_camouflage_deactivate(std::uint32_t datum,float supplied,
    const SourceCamouflageServices& services) {
  const auto speed=supplied*0.25f;
  auto state=unit(datum,services);
  state.flags&=~8u;state.flags&=~16u;state.fade_speed=speed;dirty(state,services);
}
void source_powerup_tick(std::uint32_t datum,const SourceCamouflageServices& services) {
  auto state=player(datum,services);
  for(unsigned index=0;index<2;++index)if(state.timers[index]>0) {
    --state.timers[index];
    if(state.timers[index]==0 && index==0)
      source_camouflage_deactivate(player(datum,services).current_unit,1.f,services);
  }
}
void source_camouflage_fade(std::uint32_t datum,float dt,const SourceCamouflageServices& services) {
  auto state=unit(datum,services);
  const float step=state.fade_speed*dt;
  if(state.flags&8u) {
    state.primary=step+state.primary;
    if(state.primary>1.f){state.primary=1.f;dirty(state,services);}
  } else {
    state.primary=state.primary-step;
    if(0.f>state.primary){state.primary=0.f;dirty(state,services);}
  }
  const float secondary_step=dt*0.3333333432674408f;
  if(state.flags&16u) {
    state.secondary=secondary_step+state.secondary;
    if(state.secondary>1.f)state.secondary=1.f;
  } else {
    state.secondary=state.secondary-secondary_step;
    if(0.f>state.secondary)state.secondary=0.f;
  }
}
void source_camouflage_disrupt(std::uint32_t datum,float amount,float limit,
    const SourceCamouflageServices& services) {
  auto state=unit(datum,services);
  if(state.intrinsic_mode!=0) {
    if(!services.intrinsic_control)
      throw std::runtime_error("Source intrinsic camouflage controller unavailable");
    services.intrinsic_control(datum,false);return;
  }
  if(limit==0.f)limit=0.25f;
  if(state.fade_speed<limit)limit=state.fade_speed;
  state.fade_speed=limit;
  state.primary=state.primary-amount;
  if((state.flags&8u) && state.primary<0.05f)state.primary=0.05f;
  dirty(state,services);
}
}
