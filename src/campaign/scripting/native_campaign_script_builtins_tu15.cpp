#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "native_campaign_script_fp_tu15.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t object_pool_global=0x4ecbd0u;
constexpr std::uint32_t device_group_pool_global=0x4ecbf8u;
constexpr std::uint32_t random_seed_global=0x4f3cd8u;
constexpr std::uint32_t garbage_collection_global=0x4eabc4u;
constexpr std::uint32_t physics_constants_global=0x52be68u;
constexpr std::uint32_t game_time_global=0x51df54u;
constexpr std::uint32_t player_effects_global=0x51df5cu;
constexpr std::uint32_t pvs_global=0x4f3218u;
constexpr std::uint32_t function_slots_global=0x51daf4u;
constexpr std::uint32_t interpolators_global=0x4f3010u;
constexpr std::uint32_t scenario_global=0x4ecc20u;
// Original .rdata constants, authenticated by the physical oracle.
constexpr std::uint32_t gravity_scale_bits=0x40857af5u;      // 450268: 4.1712594
constexpr std::uint32_t random_unit_bits=0x37800080u;        // 46A160: 1/65535
constexpr std::uint32_t ground_adhesion_limit_bits=0x40a00000u; // 46A100: 5.0
constexpr std::uint32_t degrees_to_radians_bits=0x3c8efa35u; // 46A164
constexpr std::uint32_t minus_one_bits=0xbf800000u;          // 46A0DC

using Bank=NativeCampaignScriptBank;
using namespace native_campaign_script_fp;

float as_float(std::uint32_t bits){return std::bit_cast<float>(bits);}
std::uint32_t as_bits(float value){return std::bit_cast<std::uint32_t>(value);}

// Unchecked low-16 object row lookup used directly by most evaluators.
std::uint32_t object_body(const Bank& bank,std::uint32_t object) {
  const auto records=bank.u32(bank.u32(object_pool_global)+0x44u);
  return bank.u32(records+(object&0xffffu)*12u+8u);
}
// Inline 11CD60 / BCDF0 salted row validation; returns the row address or 0.
std::uint32_t valid_object_row(const Bank& bank,std::uint32_t object) {
  if(object==native_campaign_script_tu15::none)return 0;
  const auto pool=bank.u32(object_pool_global);
  const auto index=object&0xffffu;
  if(std::int32_t(index)>=std::int32_t(bank.u32(pool+0x38u)))return 0;
  const auto row=bank.u32(pool+0x24u)*index+bank.u32(pool+0x44u);
  const auto salt=std::int16_t(bank.u16(row));
  if(!salt || std::int32_t(salt)!=(std::int32_t(object)>>16))return 0;
  return row;
}
// CVTSI2SS(game ticks per second) * seconds, then FLD/FISTP.
std::uint32_t game_ticks(const Bank& bank,float seconds) {
  const auto rate=std::int16_t(bank.u16(bank.u32(game_time_global)+2u));
  const auto& fp=bank.floating_point();
  return fistp32(mulss(cvtsi2ss(rate,fp.mxcsr),seconds,fp.mxcsr),fp.x87_control_word);
}
void device_flag(Bank& bank,std::uint32_t device,std::uint32_t bit,bool set,bool mark) {
  const auto body=native_campaign_script_tu15::object_try_and_get_and_verify_type(bank,device,0x80u);
  if(!body)return;
  const auto flags=bank.u32(body+0x1ccu);
  bank.store32(body+0x1ccu,set?flags|bit:flags&~bit);
  if(mark)native_campaign_script_tu15::object_mark_for_update(bank,device);
}
}

std::uint8_t NativeCampaignScriptBank::u8(std::uint32_t address)const {
  std::array<std::byte,1> bytes{};memory_.read(address,bytes);return std::to_integer<std::uint8_t>(bytes[0]);
}
std::uint16_t NativeCampaignScriptBank::u16(std::uint32_t address)const {
  std::array<std::byte,2> bytes{};memory_.read(address,bytes);
  return std::uint16_t(std::to_integer<unsigned>(bytes[0])|std::to_integer<unsigned>(bytes[1])<<8u);
}
std::uint32_t NativeCampaignScriptBank::u32(std::uint32_t address)const {
  std::array<std::byte,4> bytes{};memory_.read(address,bytes);std::uint32_t value{};
  for(unsigned n=0;n<4;++n)value|=std::uint32_t(std::to_integer<unsigned>(bytes[n]))<<(8u*n);
  return value;
}
void NativeCampaignScriptBank::store(std::uint32_t address,std::span<const std::byte> bytes) {
  if(writer_)writer_(address,bytes);else memory_.write(address,bytes);
}
void NativeCampaignScriptBank::store8(std::uint32_t address,std::uint8_t value) {
  const std::array bytes{std::byte(value)};store(address,bytes);
}
void NativeCampaignScriptBank::store16(std::uint32_t address,std::uint16_t value) {
  const std::array bytes{std::byte(value),std::byte(value>>8u)};store(address,bytes);
}
void NativeCampaignScriptBank::store64(std::uint32_t address,std::uint32_t low,std::uint32_t high) {
  std::array<std::byte,8> bytes{};
  for(unsigned n=0;n<4;++n){bytes[n]=std::byte(low>>(8u*n));bytes[4+n]=std::byte(high>>(8u*n));}
  store(address,bytes);
}
void NativeCampaignScriptBank::store32(std::uint32_t address,std::uint32_t value) {
  const std::array bytes{std::byte(value),std::byte(value>>8u),std::byte(value>>16u),std::byte(value>>24u)};
  store(address,bytes);
}

void NativeCampaignScriptScratch::read(std::uint32_t address,std::span<std::byte> output)const {
  const auto end=std::uint64_t(address)+output.size();
  if(address>=base && end<=std::uint64_t(base)+bytes_.size()) {
    std::copy_n(bytes_.begin()+(address-base),output.size(),output.begin());return;
  }
  if(end>base && address<std::uint64_t(base)+bytes_.size())
    throw std::runtime_error("Native script scratch access straddles its extent");
  world_.read(address,output);
}
void NativeCampaignScriptScratch::write(std::uint32_t address,std::span<const std::byte> input) {
  if(address<base || std::uint64_t(address)+input.size()>std::uint64_t(base)+bytes_.size())
    throw std::runtime_error("Native script scratch write left its private extent");
  std::copy(input.begin(),input.end(),bytes_.begin()+(address-base));
}

namespace native_campaign_script_tu15 {
std::uint32_t object_try_and_get_and_verify_type(const Bank& bank,std::uint32_t object,std::uint32_t type_mask) {
  const auto row=valid_object_row(bank,object);
  if(!row || !(type_mask&(1u<<(bank.u8(row+3u)&31u))))return 0;
  return bank.u32(row+8u);
}
std::uint32_t device_group_from_ordinal(const Bank& bank,std::uint16_t ordinal) {
  const auto index=std::int32_t(std::int16_t(ordinal));
  std::int32_t clamped=0;
  if(index>=0) {
    const auto last=std::int32_t(bank.u32(bank.u32(scenario_global)+0xa0u))-1;
    clamped=index>last?last:index;
  }
  if(clamped!=index)return none;
  const auto pool=bank.u32(device_group_pool_global);
  const auto salt=std::int32_t(std::int16_t(bank.u16(bank.u32(pool+0x24u)*std::uint32_t(index)+bank.u32(pool+0x44u))));
  const auto group=(std::uint32_t(salt)<<16)|std::uint32_t(index);
  if(bank.u8(bank.u32(pool+0x44u)+(group&0xffffu)*12u+2u)&4u)return none;
  return group;
}
void object_mark_for_update(Bank& bank,std::uint32_t object) {
  const auto pool=bank.u32(object_pool_global);
  auto records=bank.u32(pool+0x44u);
  // B93A6 tests the starting row before its NONE comparison.
  if(bank.u8(records+(object&0xffffu)*12u+2u)&2u)return;
  while(object!=none) {
    const auto row=(object&0xffffu)*12u;
    bank.store8(records+row+2u,std::uint8_t(bank.u8(records+row+2u)|2u));
    records=bank.u32(pool+0x44u);
    object=bank.u32(bank.u32(records+row+8u)+0x14u);
  }
}

std::uint32_t pin(std::uint32_t value_bits,std::uint32_t lower_bits,std::uint32_t upper_bits,std::uint32_t mxcsr) {
  const auto compared=[mxcsr](std::uint32_t bits) {
    // DAZ: denormal comparison operands act as zero of the same sign.
    if((mxcsr&0x40u) && !(bits&0x7f800000u))bits&=0x80000000u;
    return as_float(bits);
  };
  const float value=compared(value_bits),lower=compared(lower_bits),upper=compared(upper_bits);
  // COMISS: unordered operands select neither clamp.
  if(lower>value)return lower_bits;
  if(value>upper)return upper_bits;
  return value_bits;
}
std::uint16_t random_range(Bank& bank,std::uint16_t lower,std::uint16_t upper) {
  const auto seed=bank.u32(random_seed_global);
  const auto state=bank.u32(seed)*0x19660du+0x3c6ef35fu;
  bank.store32(seed,state);
  const auto span=std::uint32_t(std::int32_t(std::int16_t(upper))-std::int32_t(std::int16_t(lower)));
  return std::uint16_t(((span*(state>>16u))>>16u)+lower);
}
std::uint32_t real_random_range(Bank& bank,std::uint32_t lower_bits,std::uint32_t upper_bits) {
  const auto seed=bank.u32(random_seed_global);
  const auto state=bank.u32(seed)*0x19660du+0x3c6ef35fu;
  bank.store32(seed,state);
  const float lower=as_float(lower_bits),upper=as_float(upper_bits);
  return as_bits(random_interval(state>>16u,lower,upper,as_float(random_unit_bits),
      bank.floating_point().x87_control_word));
}

void garbage_collect(Bank& bank,bool unsafe) {
  const auto globals=bank.u32(garbage_collection_global);
  bank.store8(globals+1u,1);
  bank.store8(globals+2u,unsafe?1:0);
}
void physics_set_gravity(Bank& bank,std::uint32_t scale) {
  const auto value=mulss(as_float(scale),as_float(gravity_scale_bits),bank.floating_point().mxcsr);
  bank.store32(bank.u32(physics_constants_global),as_bits(value));
}
void physics_set_velocity_frame(Bank& bank,std::uint32_t i,std::uint32_t j,std::uint32_t k) {
  const auto constants=bank.u32(physics_constants_global);
  bank.store32(constants+0xcu,i);
  bank.store32(constants+0x10u,j);
  bank.store32(constants+0x14u,k);
}
void physics_disable_character_ground_adhesion_forces(Bank& bank,std::uint32_t seconds_bits) {
  float seconds=as_float(seconds_bits);
  if(0.f>seconds)seconds=0.f;
  else if(seconds>as_float(ground_adhesion_limit_bits))seconds=as_float(ground_adhesion_limit_bits);
  const auto now=bank.u32(bank.u32(game_time_global)+8u);
  const auto ticks=game_ticks(bank,seconds);
  bank.store32(bank.u32(physics_constants_global)+0x18u,ticks+now);
}
void player_effect_set_max_rotation(Bank& bank,std::uint32_t yaw,std::uint32_t pitch,std::uint32_t roll) {
  const auto mxcsr=bank.floating_point().mxcsr;const auto scale=as_float(degrees_to_radians_bits);
  const auto effects=bank.u32(player_effects_global);
  bank.store32(effects+0x14u,as_bits(mulss(as_float(yaw),scale,mxcsr)));
  bank.store32(effects+0x18u,as_bits(mulss(as_float(pitch),scale,mxcsr)));
  bank.store32(effects+0x1cu,as_bits(mulss(as_float(roll),scale,mxcsr)));
}
void player_effect_start(Bank& bank,std::uint32_t maximum_intensity,std::uint32_t attack_seconds) {
  const auto effects=bank.u32(player_effects_global);
  bank.store32(effects+0x20u,maximum_intensity);
  const auto ticks=game_ticks(bank,as_float(attack_seconds));
  const auto flags=(bank.u32(effects+0x28u)&~2u)|1u;
  bank.store16(effects+0x24u,std::uint16_t(ticks));
  bank.store16(effects+0x26u,std::uint16_t(ticks));
  bank.store32(effects+0x28u,flags);
}
void player_effect_stop(Bank& bank,std::uint32_t decay_seconds) {
  const auto ticks=game_ticks(bank,as_float(decay_seconds));
  const auto effects=bank.u32(player_effects_global);
  bank.store16(effects+0x24u,std::uint16_t(ticks));
  bank.store16(effects+0x26u,std::uint16_t(ticks));
  bank.store32(effects+0x28u,bank.u32(effects+0x28u)|2u);
}
void pvs_set_object(Bank& bank,std::uint32_t object) {
  const auto pvs=bank.u32(pvs_global);
  if(object==none){bank.store16(pvs+0x11fau,0);return;}
  bank.store32(pvs+0x11fcu,object);
  bank.store16(pvs+0x11fau,1);
}
void pvs_clear(Bank& bank) {
  bank.store16(bank.u32(pvs_global)+0x11fau,0);
}

std::uint32_t object_get_parent(const Bank& bank,std::uint32_t object) {
  return object==none?none:bank.u32(object_body(bank,object)+0x14u);
}
void object_set_phantom_power(Bank& bank,std::uint32_t object,std::uint8_t enabled) {
  if(object==none)return;
  const auto flags=object_body(bank,object)+0xc1u;
  const auto value=bank.u8(flags);
  bank.store8(flags,enabled?std::uint8_t(value|4u):std::uint8_t(value&0xfbu));
}
void object_set_deleted_when_deactivated(Bank& bank,std::uint32_t object) {
  if(object==none)return;
  const auto flags=object_body(bank,object)+4u;
  bank.store32(flags,bank.u32(flags)|0x20000u);
}
void object_set_shield_stun_infinite(Bank& bank,std::uint32_t object) {
  if(object==none)return;
  bank.store16(object_body(bank,object)+0x104u,0x7fff);
}
std::uint32_t object_get_shield(const Bank& bank,std::uint32_t object) {
  const auto body=object_try_and_get_and_verify_type(bank,object,0xffffffffu);
  if(!body)return minus_one_bits;
  if((bank.u8(body+0x10au)>>2u)&1u)return 0;
  return bank.u32(body+0xf0u);
}
std::uint32_t object_get_health(const Bank& bank,std::uint32_t object) {
  const auto body=object_try_and_get_and_verify_type(bank,object,0xffffffffu);
  if(!body)return minus_one_bits;
  if((bank.u8(body+0x10au)>>2u)&1u)return 0;
  return bank.u32(body+0xecu);
}
void object_set_shield(Bank& bank,std::uint32_t object,std::uint32_t fraction) {
  if(object==none)return;
  const auto mx=bank.floating_point().mxcsr;
  const auto body=object_body(bank,object);
  auto value=fraction;
  if(sse_compared(0,mx)>sse_compared(value,mx))value=0;
  else if(sse_compared(value,mx)>1.f)value=0x3f800000u;
  bank.store32(body+0xf0u,std::bit_cast<std::uint32_t>(mulss(std::bit_cast<float>(bank.u32(body+0xe8u)),
      std::bit_cast<float>(value),mx)));
}
void object_clear_all_function_variables(Bank& bank,std::uint32_t object) {
  if(object==none)return;
  const auto body=object_body(bank,object);
  if(!((bank.u16(body+0xc0u)>>9u)&1u))return;
  const auto slots=bank.u32(function_slots_global)+0x280u;
  for(unsigned slot=0;slot<32;++slot)
    if(bank.u32(slots+slot*4u)==object)bank.store32(slots+slot*4u,none);
  bank.store8(body+0xc1u,std::uint8_t(bank.u8(body+0xc1u)&0xfdu));
}
void unit_kill(Bank& bank,std::uint32_t unit,bool silent) {
  if(unit==none)return;
  const auto flags=object_body(bank,unit)+0x10au;
  bank.store8(flags,std::uint8_t(bank.u8(flags)|(silent?0x40u:0x20u)));
  object_mark_for_update(bank,unit);
}

void device_operates_automatically_set(Bank& bank,std::uint32_t device,std::uint8_t enabled) {
  device_flag(bank,device,1u,!enabled,true);
}
void device_closes_automatically_set(Bank& bank,std::uint32_t device,std::uint8_t enabled) {
  device_flag(bank,device,2u,!enabled,true);
}
void device_one_sided_set(Bank& bank,std::uint32_t device,std::uint8_t enabled) {
  device_flag(bank,device,4u,enabled,false);
}
void device_set_never_appears_locked(Bank& bank,std::uint32_t device,std::uint8_t enabled) {
  if(device==none)return;
  device_flag(bank,device,0x10u,enabled,false);
}
std::uint32_t device_get_position(const Bank& bank,std::uint32_t device) {
  return device==none?0:bank.u32(object_body(bank,device)+0x140u);
}
std::uint32_t device_group_get(const Bank& bank,std::uint32_t group) {
  return bank.u32(bank.u32(bank.u32(device_group_pool_global)+0x44u)+(group&0xffffu)*12u+4u);
}
namespace {
// 10C5D0: mark this device and its whole child hierarchy for an immediate
// transform refresh. The sibling link is re-read after each child's update.
void device_refresh_hierarchy(Bank& bank,std::uint32_t object) {
  const auto body=object_body(bank,object);
  if((1u<<(bank.u8(body+0xaau)&31u))&0x380u) {
    bank.store32(body+0x12cu,bank.u32(body+0x12cu)|4u);
    object_mark_for_update(bank,object);
  }
  for(auto child=bank.u32(body+0x10u);child!=none;child=bank.u32(object_body(bank,child)+0xcu))
    device_refresh_hierarchy(bank,child);
}
}
void device_group_set_immediate(Bank& bank,std::uint32_t group,std::uint32_t position) {
  if(group==none)return;
  const auto value=pin(position,0,0x3f800000u,bank.floating_point().mxcsr);
  bank.store32(bank.u32(bank.u32(device_group_pool_global)+0x44u)+(group&0xffffu)*12u+4u,value);
  // BCEE0 scans live machine/control/light-fixture rows, including high-bit
  // salts. Each matching power and position membership is visited separately.
  const auto pool=bank.u32(object_pool_global);
  std::int16_t index=0;
  for(;;) {
    const auto records=bank.u32(pool+0x44u);
    auto current=std::uint32_t(std::int32_t(index));
    if(std::int32_t(bank.u32(pool+0x38u))<=std::int32_t(current))return;
    std::uint32_t address=records+current*12u,body=0,handle=0;
    for(;;) {
      ++index;
      const auto salt=std::int16_t(bank.u16(address));
      if(salt && (0x380u&(1u<<(bank.u8(address+3u)&31u)))) {
        handle=(std::uint32_t(std::int32_t(salt))<<16u)|current;
        body=bank.u32(address+8u);
        break;
      }
      current=std::uint32_t(std::int32_t(index));address+=12u;
      if(std::int32_t(bank.u32(pool+0x38u))<=std::int32_t(current))return;
    }
    if(!body)return;
    for(const auto offset:{0x130u,0x13cu})if(bank.u32(body+offset)==group) {
      device_refresh_hierarchy(bank,handle);
      bank.store32(body+offset+4u,value);
      bank.store32(body+offset+8u,0);
      object_mark_for_update(bank,handle);
    }
  }
}
void device_set_position_immediate(Bank& bank,std::uint32_t device,std::uint32_t position) {
  if(device==none)return;
  const auto body=object_body(bank,device),group=bank.u32(body+0x13cu);
  if(group!=none)device_group_set_immediate(bank,group,position);
  bank.store32(body+0x12cu,bank.u32(body+0x12cu)|0x80u);
  object_mark_for_update(bank,device);
}
void device_group_change_only_once_more_set(Bank& bank,std::uint32_t group,std::uint8_t enabled) {
  if(group==none)return;
  const auto row=bank.u32(bank.u32(device_group_pool_global)+0x44u)+(group&0xffffu)*12u+2u;
  bank.store8(row,enabled?std::uint8_t(bank.u8(row)|1u):std::uint8_t(bank.u8(row)&0xfeu));
  bank.store8(row,std::uint8_t(bank.u8(row)&0xfdu));
  // BCEE0 iteration over machines/controls/light fixtures (type mask 0x380).
  const auto pool=bank.u32(object_pool_global);
  std::int16_t index=0;
  for(;;) {
    const auto records=bank.u32(pool+0x44u);
    auto current=std::uint32_t(std::int32_t(index));
    if(std::int32_t(bank.u32(pool+0x38u))<=std::int32_t(current))return;
    std::uint32_t address=records+current*12u,body=0,handle=0;
    for(;;) {
      ++index;
      const auto salt=std::int16_t(bank.u16(address));
      if(salt && (0x380u&(1u<<(bank.u8(address+3u)&31u)))) {
        handle=(std::uint32_t(std::int32_t(salt))<<16u)|current;
        body=bank.u32(address+8u);
        break;
      }
      current=std::uint32_t(std::int32_t(index));address+=12u;
      if(std::int32_t(bank.u32(pool+0x38u))<=std::int32_t(current))return;
    }
    if(!body)return;
    if(bank.u32(body+0x130u)==group)object_mark_for_update(bank,handle);
    if(bank.u32(body+0x13cu)==group)object_mark_for_update(bank,handle);
  }
}

bool unit_in_vehicle(const Bank& bank,std::uint32_t unit) {
  return unit!=none && bank.u16(object_body(bank,unit)+0x1fcu)!=0xffffu;
}
void unit_only_takes_damage_from_players_team(Bank& bank,std::uint32_t unit,std::uint8_t enabled) {
  if(unit==none)return;
  const auto flags=object_body(bank,unit)+0x134u;
  const auto value=bank.u32(flags);
  bank.store32(flags,enabled?value|0x80000000u:value&0x7fffffffu);
}
void unit_doesnt_drop_items(Bank& bank,std::span<const std::uint32_t> members) {
  for(const auto member:members) {
    if(member==none)return;
    const auto row=valid_object_row(bank,member);
    if(!row || !((1u<<(bank.u8(row+3u)&31u))&3u))continue;
    const auto body=bank.u32(row+8u);
    if(!body)continue;
    bank.store32(body+0x134u,bank.u32(body+0x134u)|0x10000u);
  }
}
bool unit_is_emitting(const Bank& bank,std::uint32_t unit) {
  if(unit==none)return false;
  const auto body=object_body(bank,unit);
  if((bank.u8(body+0x10au)>>2u)&1u)return false;
  const auto offset=std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x346u))));
  return (bank.u32(body+offset+8u)>>18u)&1u;
}
std::uint32_t vehicle_driver(const Bank& bank,std::uint32_t unit) {
  const auto body=object_try_and_get_and_verify_type(bank,unit,3u);
  return body?bank.u32(body+0x248u):none;
}

std::uint32_t interpolator_start(Bank& bank,std::uint32_t name,std::uint32_t target,std::uint32_t seconds) {
  // 137800: first nonzero matching scenario row; the match publishes the
  // looked-up name at the state bank head before returning its record.
  const auto state=bank.u32(interpolators_global);
  if(!state)return none;
  const auto scenario=bank.u32(scenario_global);
  if(!scenario || !name)return none;
  const auto count=std::int32_t(bank.u32(scenario+0x3c0u));
  if(count<=0)return none;
  const auto rows=bank.u32(scenario+0x3c4u);
  std::int32_t index=0;
  for(;index<count;++index) {
    const auto row=bank.u32(rows+std::uint32_t(index)*0x18u);
    if(row && row==name)break;
  }
  if(index>=count)return none;
  bank.store32(state,name);
  const auto record=state+std::uint32_t(index)*32u+4u;
  // 137C04..137C50.
  const auto mxcsr=bank.floating_point().mxcsr;
  const auto time=bank.u32(game_time_global);
  float now=0.f;
  if(time && bank.u8(time))now=mulss(cvtsi2ss(std::int32_t(bank.u32(time+8u)),mxcsr),as_float(bank.u32(time+4u)),mxcsr);
  const auto flags=std::uint8_t((bank.u8(record+0x1cu)&0xfdu)|1u);
  const auto current=bank.u32(record);
  bank.store32(record+0xcu,as_bits(now));
  bank.store32(record+0x10u,as_bits(now));
  const auto end=addss(now,as_float(seconds),mxcsr);
  bank.store8(record+0x1cu,flags);
  bank.store32(record+4u,current);
  bank.store32(record+8u,target);
  bank.store32(record+0x14u,as_bits(end));
  bank.store32(record+0x18u,0);
  return std::uint32_t(index);
}

std::uint16_t list_count_not_dead(const Bank& bank,std::span<const std::uint32_t> members) {
  std::uint16_t count=0;
  for(const auto member:members) {
    if(member==none)break;
    if(!((bank.u8(object_body(bank,member)+0x10au)>>2u)&1u))++count;
  }
  return count;
}
}

bool native_campaign_script_noop_builtin_tu15(std::string_view name) {
  // 2AC980 is shared by all three descriptors and immediately returns void.
  return name=="cheat_active_camouflage_by_player" || name=="time_code_show" || name=="time_code_start";
}
}
