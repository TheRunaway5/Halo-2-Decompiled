#include "halo2/weapon_magazine.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
std::int16_t short_word(std::uint32_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
void require(bool value,const char *message) {
  if(!value)throw std::runtime_error(message);
}
}
std::int32_t weapon_reserve_available(const WeaponAmmunitionContext &context,
    unsigned magazine,bool self_only) {
  const auto reserve=[&](const WeaponAmmunitionView &view) -> std::int32_t {
    require(magazine<view.magazines.size() && magazine<view.definitions.size(),
        "Source reserve query requires a valid authored magazine");
    return context.infinite?view.definitions[magazine].maximum_reserve:view.magazines[magazine].reserve;
  };
  auto result=reserve(context.current);
  if(!context.has_unit || self_only)return result;
  for(const auto &view:context.inventory)
    if(view.datum!=0xffffffffu && view.datum!=context.current.datum &&
        view.definition==context.current.definition)result+=reserve(view);
  return result;
}
bool weapon_reserve_spend(WeaponAmmunitionContext &context,unsigned magazine,
    std::int32_t amount,const std::function<void()> &changed) {
  if(amount>weapon_reserve_available(context,magazine))return false;
  auto &own=context.current.magazines[magazine].reserve;
  auto spent=std::min<std::int32_t>(own,amount);
  own=short_word(std::uint32_t(std::int32_t(own))-std::uint32_t(spent));
  if(spent>0) {
    require(bool(changed),"Source reserve changed notification unavailable");changed();
  }
  if(context.has_unit && spent<amount)
    for(auto &view:context.inventory)
      if(view.datum!=0xffffffffu && view.datum!=context.current.datum &&
          view.definition==context.current.definition) {
        require(magazine<view.magazines.size(),"Source reserve spend requires a valid authored magazine");
        auto &remaining=view.magazines[magazine].reserve;
        const auto take=std::min<std::int32_t>(remaining,amount-spent);
        if(take>0) {
          remaining=short_word(std::uint32_t(std::int32_t(remaining))-std::uint32_t(take));
          spent+=take;
        }
      }
  return true;
}
void weapon_magazine_transfer(WeaponMagazineState &s,const WeaponMagazineDefinition &d,
    std::uint32_t item_flags,const WeaponMagazineTransferServices &services) {
  if(d.flags&1u)s.loaded=0;
  require(bool(services.available),"Source magazine ammunition owner unavailable");
  const auto amount=std::min<std::int32_t>(d.reload_amount,services.available());
  const auto before=std::uint16_t(s.loaded);
  auto after=std::uint32_t(before)+std::uint32_t(amount);
  if(short_word(after)>d.capacity)after=std::uint32_t(std::int32_t(d.capacity));
  if(item_flags&8u) {
    require(bool(services.spend),"Source magazine ammunition spend owner unavailable");
    services.spend(std::int32_t(short_word(after))-std::int32_t(short_word(before)));
  }
  s.loaded=short_word(after);s.transfer_ticks=s.other_ticks=-1;
  require(bool(services.changed),"Source magazine changed notification unavailable");
  services.changed();
}
void weapon_magazine_transition(WeaponMagazineState &s,const WeaponMagazineDefinition &d,
    std::int16_t requested,std::int16_t tick_rate,const WeaponMagazineTransitionServices &services) {
  require(requested>=0 && requested<=6,"Source magazine mode exceeds original dispatch table");
  std::int32_t ticks=-1,transfer=-1,action=-1;
  if(requested==6) {
    const float seconds=float(tick_rate)*d.chamber_seconds;
    const float rounded=std::nearbyint(seconds);
    ticks=rounded>=-2147483648.f && rounded<2147483648.f
        ?std::int32_t(rounded):std::int32_t(-2147483647-1);
  } else if(requested>=1 && requested<=4) {
    const bool empty=s.loaded==0;
    if(requested==1)action=empty?7:8;
    else if(requested==2)action=9;
    else if(requested==3)action=empty?10:11;
    if(action!=-1) {
      require(bool(services.activity_symbol) && bool(services.timing),
          "Source magazine animation lookup/timing owner unavailable");
      const auto symbol=services.activity_symbol(unsigned(action));
      ticks=services.timing(symbol,0);
      if(requested!=2)transfer=services.timing(symbol,3);
    }
  }
  s.mode=requested;s.ticks=short_word(std::uint32_t(ticks));
  s.transfer_ticks=s.other_ticks=short_word(std::uint32_t(transfer));
  if(action!=-1) {
    require(bool(services.activity),"Source magazine activity notification unavailable");
    services.activity(unsigned(action));
  }
}
bool weapon_magazine_reload(WeaponMagazineState &s,const WeaponMagazineDefinition &d,
    std::int16_t weapon_type,unsigned magazine,std::int16_t tick_rate,const WeaponMagazineServices &services) {
  const std::int16_t next=weapon_type==1?((s.mode==3 || s.mode==4)?3:2):1;
  if(s.mode!=0 && s.mode!=5 && !(s.mode==4 && next==3))return false;
  require(bool(services.idle),"Source reload idle owner unavailable");
  if(!services.idle())return false;
  require(bool(services.transfer.available),"Source reload reserve owner unavailable");
  if(services.transfer.available()<=0 || s.loaded>=d.capacity)return false;
  require(bool(services.request_state) && bool(services.effect),"Source reload state/effect owners unavailable");
  services.request_state(magazine+5);
  weapon_magazine_transition(s,d,next,tick_rate,services.transition);
  services.effect(d.reload_effect);
  return true;
}
void weapon_magazine_chamber(WeaponMagazineState &s,const WeaponMagazineDefinition &d,
    unsigned magazine,std::int16_t tick_rate,const WeaponMagazineServices &services) {
  if(s.mode!=0 && s.mode!=5)return;
  require(bool(services.idle),"Source chamber idle owner unavailable");
  if(!services.idle())return;
  require(bool(services.effect) && bool(services.request_state),"Source chamber effect/state owners unavailable");
  services.effect(d.chamber_effect);services.request_state(magazine+3);
  weapon_magazine_transition(s,d,6,tick_rate,services.transition);
}
bool weapon_magazine_step(WeaponMagazineState &s,const WeaponMagazineDefinition &d,
    std::uint32_t item_flags,std::uint16_t input_flags,unsigned magazine,
    std::int16_t tick_rate,const WeaponMagazineServices &services) {
  bool work=false;
  if(d.recharge_per_second>0 && s.loaded<d.capacity) {
    require(tick_rate!=0,"Source magazine recharge requires a nonzero tick rate");
    s.recharge=short_word(std::uint32_t(std::int32_t(s.recharge)+d.recharge_per_second%tick_rate));
    s.loaded=short_word(std::uint32_t(std::int32_t(s.loaded)+d.recharge_per_second/tick_rate));
    if(s.recharge>=tick_rate) {
      s.loaded=short_word(std::uint32_t(std::int32_t(s.loaded)+1));
      s.recharge=short_word(std::uint32_t(std::int32_t(s.recharge)-tick_rate));
    }
    if(s.loaded>d.capacity)s.loaded=d.capacity;
    require(bool(services.transfer.changed),"Source magazine recharge notification unavailable");
    services.transfer.changed();work=true;
  }
  if(s.ticks>0){--s.ticks;work=true;}
  if(s.transfer_ticks>0){--s.transfer_ticks;work=true;}
  if(s.mode!=0)work=true;
  if(s.mode>=1 && s.mode<=3) {
    if(s.ticks<s.transfer_ticks)s.transfer_ticks=s.ticks;
    if(s.transfer_ticks==0)weapon_magazine_transfer(s,d,item_flags,services.transfer);
    if(s.ticks!=0)return work;
    require(bool(services.transfer.available),"Source completed reload reserve owner unavailable");
    const auto available=services.transfer.available();
    weapon_magazine_transition(s,d,s.mode==1?0:4,tick_rate,services.transition);
    bool primary=false,secondary=false;
    if(input_flags&2u) {
      require(bool(services.fire_admitted),"Source reload primary firing admission unavailable");
      primary=services.fire_admitted(0);
    }
    if(input_flags&4u) {
      require(bool(services.fire_admitted),"Source reload secondary firing admission unavailable");
      secondary=services.fire_admitted(1);
    }
    if(available>0 && s.loaded<d.capacity && !(d.flags&1u) && !primary && !secondary) {
      require(bool(services.request_reload),"Source repeated reload action owner unavailable");
      if(services.request_reload())return work;
    }
    //106E10/107D30: action notification precedes the final mode5 stores.
    if(primary || secondary || s.mode!=4) {
      require(bool(services.transition.activity),"Source reload completion activity unavailable");
      services.transition.activity(!primary && !secondary && s.mode==3?12:0);
    }
    weapon_magazine_transition(s,d,5,tick_rate,services.transition);
  } else if(s.mode==5)weapon_magazine_chamber(s,d,magazine,tick_rate,services);
  else if(s.mode==6 && s.ticks==0)weapon_magazine_transition(s,d,0,tick_rate,services.transition);
  return work;
}
} // namespace halo2
