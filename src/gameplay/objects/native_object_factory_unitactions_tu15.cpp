// Native TU1.5 unit action system (track "unitactions"). Each owner
// replicates its original routine; oracle cases live in
// tools/native_object_factory_cases_unitactions.py and the harness kernels in
// tests/native_object_factory_unitactions_kernels.cpp. See
// docs/native_object_factory_tracks/unitactions.md.
#include "halo2/native_object_factory_unitactions_tu15.hpp"
#include "halo2/native_campaign_unit_action_support_tu15.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include "halo2/native_object_factory_units_tu15.hpp"
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "halo2/native_tu15_unit_animation_keys_kernel.hpp"
#include <algorithm>
#include <cstdio>
#include <initializer_list>

namespace halo2::native_object_factory_tu15::unit_actions {
using namespace detail;
using u32=std::uint32_t;
using i32=std::int32_t;

// BEGIN 0x473D28 {descriptor, start, update, stop, interrupt}
const std::array<Descriptor,action_count>& action_table() {
  static constexpr std::array<Descriptor,action_count> table{{
  {0x473ac4u,0xe97d0u,0xe9870u,0x0u,0xe9910u},{0x473ac4u,0xe97d0u,0xe9870u,0x0u,0xe9910u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},
  {0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},
  {0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ae4u,0xeaf00u,0xeb0a0u,0xeb290u,0xeb190u},
  {0x473af4u,0xeb360u,0x0u,0x0u,0x0u},{0x473ac4u,0xe97d0u,0xe9870u,0x0u,0xe9910u},{0x473ac4u,0xe97d0u,0xe9870u,0x0u,0xe9910u},
  {0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},
  {0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},
  {0x473ae4u,0xeaf00u,0xeb0a0u,0xeb290u,0xeb190u},{0x473af4u,0xeb360u,0x0u,0x0u,0x0u},{0x473b04u,0xeb580u,0x0u,0x0u,0x0u},
  {0x473b14u,0xeb710u,0x0u,0x0u,0x0u},{0x473b24u,0xea500u,0xea8d0u,0x0u,0xea970u},{0x473b34u,0xeb8f0u,0x0u,0x0u,0x0u},
  {0x473b44u,0xeba80u,0x0u,0x0u,0x0u},{0x473b54u,0xebc10u,0x0u,0x0u,0x0u},{0x473b64u,0xebdb0u,0x0u,0x0u,0x0u},
  {0x473b74u,0xec450u,0xec620u,0x0u,0xec750u},{0x473b84u,0xecdc0u,0xecd50u,0xecc40u,0x0u},{0x473b94u,0xed110u,0xed440u,0xed3f0u,0x0u},
  {0x473ba4u,0xed430u,0x0u,0x0u,0x0u},{0x473bb4u,0xed640u,0xed8f0u,0xed820u,0x0u},{0x473bc4u,0xed970u,0xedb50u,0xedad0u,0x8fdb0u},
  {0x473bd4u,0xedb80u,0x0u,0x0u,0x0u},{0x473be4u,0xedd90u,0x0u,0xedf10u,0x0u},{0x473bf4u,0xee050u,0x0u,0xeef30u,0x0u},
  {0x473c04u,0xee0f0u,0x0u,0x0u,0x0u},{0x473c14u,0xee2e0u,0x0u,0x0u,0x0u},{0x473c24u,0xee4d0u,0x0u,0x0u,0x0u},
  {0x473c34u,0xee600u,0x0u,0xeef30u,0x0u},{0x473c44u,0xee860u,0x0u,0xee8c0u,0x0u},{0x473c54u,0xee910u,0xeea80u,0x0u,0x0u},
  {0x473c64u,0xeeed0u,0x0u,0xeef30u,0x0u},{0x473c74u,0xeef00u,0x0u,0xeef30u,0x0u},{0x473c84u,0xeef50u,0x0u,0xef0d0u,0x0u},
  {0x473c94u,0xef270u,0x0u,0x0u,0x0u},{0x473ca4u,0xef300u,0x0u,0xef370u,0x0u},{0x473cb4u,0xef130u,0x0u,0xef220u,0x0u},
  {0x473cc4u,0xef4e0u,0x0u,0xef570u,0x0u},{0x473cd4u,0xef5a0u,0xefb10u,0x0u,0x0u},{0x473ce4u,0xefc30u,0xefcc0u,0x0u,0x0u},
  {0x473cf4u,0xefdb0u,0xefec0u,0x0u,0x0u},{0x473d04u,0xeffe0u,0x0u,0x0u,0x0u},{0x473d14u,0xf0410u,0xf04c0u,0xf0510u,0x0u},
  {0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},
  {0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},{0x473ad4u,0xe9990u,0x0u,0x0u,0x0u},
  }};
  return table;
}
// END 0x473D28

namespace {
std::string hex_name(u32 address) {
  char text[16];std::snprintf(text,sizeof text,"%X",unsigned(address));return text;
}
[[noreturn]] void refuse_callback(u32 callback,u32 type,const char* kind) {
  refuse(hex_name(callback).c_str(),"unit action "+hex_name(type)+" "+kind+" callback (0x473D28)");
}
const Descriptor& descriptor(u32 type,const char* dispatcher) {
  if(type>=action_count)refuse(dispatcher,"an action type outside 0x473D28 ("+hex_name(type)+")");
  return action_table()[type];
}
// The action block: body + signed word +346; its +4 words are the action bits.
u32 action_block(const M& m,u32 body){return body+sx16(r16(m,body+0x346u));}
u32 action_word(u32 block,u32 type){return block+u32(i32(type)>>5)*4u+4u;}
u32 weapon_in_slot(const M& m,u32 body,u32 slot_byte) {
  const auto slot=std::int8_t(r8(m,body+slot_byte));
  return slot==-1?none:r32(m,body+0x218u+u32(i32(slot))*4u);
}

std::uint8_t run_start(M& m,u32 callback,u32 unit,u32 type,const Request& r,const Fp& fp) {
  switch(callback) {
  case 0xeaf00u:return weapon_ready_start(m,unit,r,fp);
  case 0xeb360u:return weapon_put_away_start(m,unit,r,fp);
  case 0xed110u:return vehicle_exit_start(m,unit,r,fp);
  default:refuse_callback(callback,type,"start");
  }
}
std::uint8_t run_update(M& m,u32 callback,u32 unit,u32 type,const Fp&) {
  switch(callback) {
  case 0xe9870u:return weapon_trigger_update(m,unit,type);
  default:refuse_callback(callback,type,"update");
  }
}
void run_stop(M&,u32 callback,u32,u32 type,const Fp&){refuse_callback(callback,type,"stop");}
void run_interrupt(M& m,u32 callback,u32 unit,u32 type,const Fp& fp) {
  switch(callback) {
  case 0x8fdb0u:return;  // RET 8
  case 0xe9910u:return weapon_trigger_interrupt(m,unit,type);
  case 0xeb190u:return weapon_ready_interrupt(m,unit,type,fp);
  case 0xea970u:return weapon_pickup_interrupt(m,unit,fp);
  case 0xec750u:return action_target_clear(m,unit);
  default:refuse_callback(callback,type,"interrupt");
  }
}

// A refusal reached from a boolean oracle kernel names the original's AL at
// the refused callee's entry (the harness compares it).
[[noreturn]] void refuse_at(const char* owner,const std::string& why,std::uint8_t al) {
  refuse(owner,why+" (original AL "+hex_name(al)+")");
}

// ---- EAA60/EAC70 callees that are not native yet ----
std::uint8_t weapon_lower(M&,u32,std::uint8_t flag4) {
  refuse_at("105200","weapon lowering (from EAA60)",flag4);
}
void weapon_holster(M&,u32,bool primary) {
  refuse_at("EA9B0","unit weapon holster (from EAA60)",primary?1u:0u);
}
void unit_weapon_channel_clear(M&,u32) {
  refuse("CABC0","unit weapon channel clear");
}
void unit_weapon_event(M&,u32,u32) {
  refuse("F0D50","unit weapon event");
}
std::uint8_t unit_weapon_state_start(M&,u32,u32,u32) {
  refuse("116570","unit weapon animation state (111B10)");
}
void weapon_unready(M&,u32,bool,std::uint8_t al) {
  refuse_at("EAB70","unit weapon unready (from EAC70)",al);
}
void weapon_ready_begin(M&,u32,bool,u32 class0,u32) {
  refuse_at("112420","unit weapon ready animation (from EAC70)",std::uint8_t(class0));
}
void weapon_switch_effects(M&,u32,u32,std::uint8_t al) {
  refuse_at("D2DB0","unit weapon switch effects (from EAC70)",al);
}
void weapon_ready_state(M&,u32,std::uint8_t,std::uint8_t) {
  refuse("105010","weapon ready state (from EAC70)");
}

// E9E50 (stack unit, three flags; EA970 passes 1, 0, 0): without a held
// object (action block +10) block +C = 3. A held object's release clears
// block +10/+14 and the object's +4 bit 17 and +C0 bit 1, then needs BBAC0.
void held_object_release(M& m,u32 unit) {
  const u32 body=object_body(m,unit);
  const u32 block=action_block(m,body);
  const u32 held=r32(m,block+0x10u);
  if(held==none){w8(m,block+0xcu,3);return;}
  const u32 held_body=object_body(m,held);
  w32(m,block+0x10u,none);
  w16(m,block+0x14u,0xffffu);
  const u32 flags=r32(m,held_body+4u);
  w32(m,held_body+4u,flags&~0x20000u);
  w8(m,held_body+0xc0u,r8(m,held_body+0xc0u)&0xfdu);
  refuse_at("BBAC0","held object detach of "+hex_name(held)+" (from E9E50)",std::uint8_t((flags>>17)&1u));
}

bool listed(u32 key,std::initializer_list<u32> keys){return std::find(keys.begin(),keys.end(),key)!=keys.end();}
}

// ---- Dispatchers ----
void weapon_ready_interrupt(M& m,u32 unit,u32 type,const Fp& fp) {
  const u32 body=object_body(m,unit),block=action_block(m,body);
  if(!(r32(m,action_word(block,type))&(1u<<(type&31u))))return;
  const bool primary=type==8;
  const u32 weapon=weapon_in_slot(m,object_body(m,unit),primary?0x212u:0x213u);
  if(weapon!=none) {
    const auto state=unit_animation_state_active(m,unit,primary?0x8000025u:0xd000539u);
    // 105200 takes flag 1 on the stack, but AL still holds 116720's answer.
    refuse_at("105200","weapon lowering (from EB190)",state);
  }
  weapon_ready_animation(m,unit,primary,0x7000101u,0,0,fp);
}

std::uint8_t weapon_put_away_start(M& m,u32 unit,const Request& r,const Fp& fp) {
  const bool primary=r.type()==9;
  const u32 body=object_body(m,unit);
  const u32 weapon=weapon_in_slot(m,body,primary?0x212u:0x213u);
  if(weapon==none)return 0;
  if(primary)(void)unit_dual_wielding(m,unit); // CFB40's policy feeds the refused D0E10 tail.
  const auto other_slot=std::int8_t(r8(m,body+(primary?0x213u:0x212u)));
  if(other_slot!=-1) {
    const u32 other=r32(m,body+0x218u+u32(i32(other_slot))*4u);
    if(other!=none && r32(m,object_body(m,weapon))==r32(m,object_body(m,other)))
      refuse_at("106810","matching weapon put-away exchange (from EB360)",std::uint8_t(other));
  }
  for(const u32 type:{8u,0x12u,0u,0xau,0x1bu,0x16u})interrupt(m,unit,type,fp);
  weapon_holster(m,unit,primary);
  refuse("D0E10","weapon put-away inventory update (from EB360)");
}

void weapon_channels_clear(M& m,u32 unit) {
  const u32 body=object_body(m,unit),aim=body+sx16(r16(m,body+0x33eu));
  w32(m,aim+0x9cu,none);w16(m,aim+0xa0u,0xffffu);w16(m,aim+0xa2u,0xffffu);
  w32(m,aim+0xb8u,0);w32(m,aim+0xb4u,0x3f800000u);w32(m,aim+0xa4u,none);
  w8(m,aim+0xa8u,0xffu);w8(m,aim+0xa9u,0xffu);w16(m,aim+0xaau,0xffffu);
  w8(m,aim+0xacu,0);w8(m,aim+0xadu,0);w16(m,aim+0xaeu,0);w16(m,aim+0xb0u,0);w16(m,aim+0xb2u,0);
  w32(m,aim+0xbcu,none);w16(m,aim+0xc0u,0xffffu);w16(m,aim+0xc2u,0xffffu);w32(m,aim+0xd8u,0);
  w8(m,aim+0xccu,0);w8(m,aim+0xcdu,0);w16(m,aim+0xceu,0);w32(m,aim+0xd4u,0x3f800000u);
  w16(m,aim+0xd0u,0);w16(m,aim+0xd2u,0);w32(m,aim+0xc4u,none);
  w8(m,aim+0xc8u,0xffu);w8(m,aim+0xc9u,0xffu);w16(m,aim+0xcau,0xffffu);
}

void seat_weapon_actions_reset(M& m,u32 unit,const Fp& fp) {
  for(const u32 type:{8u,0x12u,0u,0xau,0x1bu})interrupt(m,unit,type,fp);
  request(m,unit,Request::of(0x13),fp);
  weapon_channels_clear(m,unit);
}

std::uint8_t request(M& m,std::uint32_t unit,const Request& r,const Fp& fp) {
  const u32 type=r.type();
  const u32 start=descriptor(type,"E8E50").start;
  native_tu15_wake_object_ancestors(m,unit);
  const std::uint8_t result=run_start(m,start,unit,type,r,fp);
  if(unit!=none) {
    const u32 player=r32(m,object_body(m,unit)+0x13cu);
    if(player!=none)
      native_campaign_unit_notify_action_tu15(m,player,{[type]{return std::uint16_t(type);},[result]{return result;}});
  }
  return result;
}

std::uint8_t request_type(M& m,std::uint32_t unit,std::uint32_t type,const Fp& fp) {
  return request(m,unit,Request::of(type),fp);
}

std::uint8_t update(M& m,std::uint32_t unit,const Fp& fp) {
  const u32 block=action_block(m,object_body(m,unit));
  std::uint8_t any=0;
  for(u32 type=0;type<action_count;++type) {
    const u32 word=action_word(block,type),bit=1u<<(type&31u);
    if(!(r32(m,word)&bit))continue;
    const u32 callback=action_table()[type].update;
    if(!callback)continue;
    if(run_update(m,callback,unit,type,fp))any=1;
    else w32(m,word,r32(m,word)&~bit);
  }
  return any;
}

void stop(M& m,std::uint32_t unit,std::uint32_t type,const Fp& fp) {
  const u32 block=action_block(m,object_body(m,unit));
  const u32 callback=descriptor(type,"E8EB0").stop;
  if(callback)run_stop(m,callback,unit,type,fp);
  const u32 word=action_word(block,type);
  w32(m,word,r32(m,word)&~(1u<<(type&31u)));
}

void interrupt(M& m,std::uint32_t unit,std::uint32_t type,const Fp& fp) {
  const u32 block=action_block(m,object_body(m,unit));
  const u32 callback=descriptor(type,"E8F10").interrupt;
  if(callback)run_interrupt(m,callback,unit,type,fp);
  const u32 word=action_word(block,type);
  w32(m,word,r32(m,word)&~(1u<<(type&31u)));
}

bool active(const M& m,std::uint32_t unit,std::uint32_t type) {
  return (r32(m,action_word(action_block(m,object_body(m,unit)),type))&(1u<<(type&31u)))!=0;
}

// ---- Callbacks ----
std::uint8_t weapon_ready_start(M& m,std::uint32_t unit,const Request& r,const Fp& fp) {
  const u32 body=object_body(m,unit);
  const u32 block=action_block(m,body);
  const bool primary=r.type()==8u;
  const u32 k=primary?0u:1u;
  if(!r.bytes[4]) {
    if(r8(m,body+k+0x216u)==r8(m,body+k+0x212u))return 0;
    if(weapon_change_blocked(m,unit))return 0;
    if((r32(m,object_body(m,unit)+0x134u)>>25)&1u)return 0;
    if(active(m,unit,0x16u))return 0;
  }
  const u32 weapon=weapon_in_slot(m,object_body(m,unit),k+0x216u);
  if(r8(m,body+k+0x216u)!=0xffu) {
    if(weapon==none)return 0;
    if(!weapon_ready_allowed(m,unit,weapon,none))return 0;
  }
  for(const u32 type:{0u,0xau,0x1bu,0x16u})interrupt(m,unit,type,fp);
  if(weapon_select(m,unit,r.bytes[4],r.bytes[5],primary,fp)) {
    weapon_ready_animation(m,unit,primary,0x7000101u,r.bytes[5],r.bytes[5],fp);
    return 1;
  }
  const u32 type=primary?8u:0x12u,word=action_word(block,type);
  w32(m,word,r32(m,word)|(1u<<(type&31u)));
  return 1;
}

// ---- Weapon readiness owners ----
std::uint8_t unit_animation_keys(const M& m,std::uint32_t unit,std::uint32_t& mode,std::uint32_t& state) {
  return native_unit_animation_keys_detail::lookup(m,unit,[&](u32,u32 value){mode=value;},[&](u32,u32 value){state=value;});
}

std::uint8_t unit_animation_state_active(const M& m,std::uint32_t unit,std::uint32_t key) {
  const u32 body=object_body(m,unit);
  const u32 aim=body+sx16(r16(m,body+0x33eu));
  if(r32(m,aim+0xa4u)==key || r32(m,aim+0xc4u)==key)return 1;
  u32 mode,state;
  return unit_animation_keys(m,unit,mode,state) && state==key;
}

bool unit_animation_mode_locked(const M& m,std::uint32_t unit) {
  if(unit_animation_mode_listed(m,unit))return true;
  const u32 body=object_body(m,unit);
  return listed(r32(m,body+sx16(r16(m,body+0x12au))+0x7cu),{0x80000c4u,0x400004au,0x50000c3u});
}

std::uint8_t weapon_change_blocked(const M& m,std::uint32_t unit) {
  // 113190's compare trees: "overlay" keys answer !overlay, "unloaded" keys
  // answer !unloaded.
  static constexpr std::initializer_list<u32> state_overlay{
    0x400004au,0x400076cu,0x400076du,0x50000c3u,0x5000768u,0x5000769u,0x500076au,0x500076bu,0x600005fu,0x600008cu,
    0x60000ccu,0x60000cdu,0x60006acu,0x700002cu,0x7000543u,0x700076eu,0x80000c4u,0x800076fu,0x9000011u,0x9000012u,
    0xa000010u,0xa000013u,0xa00002du,0xa00003eu,0xa00022du,0xa000767u,0xb00002eu,0xb00022eu,0xb0005b8u,0xc000043u,
    0xc0005b7u,0xd00002bu,0xd000042u,0xd00022cu,0xe00002au,0xe00003bu,0xe0000c3u,0xf00003au,0x1000003cu};
  static constexpr std::initializer_list<u32> state_unloaded{
    0x500000au,0x5000024u,0xa000040u,0xa0005b9u,0xb0005b2u,0xc000073u,0xc000075u,0xc000077u,0xc0006b3u,0xc0006cdu,
    0xd000021u,0xe000038u,0xe00067du,0x1000006cu,0x11000074u,0x11000076u,0x140005b3u};
  static constexpr std::initializer_list<u32> aim_overlay{0xa00022du,0xb00022eu,0xd00022cu};
  static constexpr std::initializer_list<u32> aim_unloaded{
    0x500000au,0xa000040u,0xa0005b9u,0xb0005b2u,0xc000073u,0xc000075u,0xc000077u,0xc0006b3u,0xc0006cdu,0xd000021u,
    0xe000038u,0xe00067du,0x1000006cu,0x11000074u,0x11000076u,0x140005b3u};
  const auto in=[](u32 key,const std::initializer_list<u32>& keys){return std::find(keys.begin(),keys.end(),key)!=keys.end();};
  const u32 body=object_body(m,unit);
  const u32 aim_key=r32(m,body+sx16(r16(m,body+0x33eu))+0xa4u);
  const u32 manager=body+sx16(r16(m,body+0x12au));
  const u32 state=r32(m,manager+0x7cu);
  bool unloaded=false,overlay=false;
  if(r32(m,body+0x13cu)!=none) {
    const u32 primary=weapon_in_slot(m,body,0x212u);
    if(primary!=none)unloaded=std::int16_t(r16(m,object_body(m,primary)+0x17cu))<=0;
    const u32 secondary=weapon_in_slot(m,body,0x213u);
    if(secondary!=none && std::int16_t(r16(m,object_body(m,secondary)+0x17cu))>0)unloaded=false;
    overlay=(r8(m,manager+0x14u)&0x10u)!=0;
  }
  std::uint8_t blocked=0;
  if(unit_animation_mode_locked(m,unit))blocked=1;
  else if(in(state,state_overlay))blocked=!overlay;
  else if(in(state,state_unloaded))blocked=!unloaded;
  if(in(aim_key,aim_overlay))blocked=!overlay;
  else if(in(aim_key,aim_unloaded))blocked=!unloaded;
  return blocked;
}

std::uint8_t weapon_animation_available(const M& m,std::uint32_t unit,std::uint32_t key,std::uint32_t class0,
    std::uint32_t class1) {
  const u32 body=object_body(m,unit);
  const u32 manager=body+sx16(r16(m,body+0x12au));
  weapon_animation::FrameWord selected[4],resolved[4],result(none);
  weapon_animation::choose_animation(m,manager,selected,resolved,key,class0,class1,0x400000cu,2u,result);
  return (result()>>16)!=0xffffu;
}

std::uint8_t weapon_ready_allowed(const M& m,std::uint32_t unit,std::uint32_t weapon,std::uint32_t key) {
  const u32 body=object_body(m,unit);
  const u32 definition=tag_data(m,r32(m,object_body(m,weapon)));
  u32 seat_key=0x7000101u;
  bool seat_refuses=false;
  const u32 parent=r32(m,body+0x14u);
  if(parent!=none) {
    const std::uint16_t seat=r16(m,body+0x1fcu);
    if(seat!=0xffffu) {
      const u32 row=r32(m,tag_data(m,r32(m,object_body(m,parent)))+0x1ccu)+sx16(seat)*0xb0u;
      seat_refuses=!((r32(m,row)>>5)&1u);
      seat_key=r32(m,row+4u);
    }
  }
  if(key==none || key==0x7000101u)key=seat_key;
  std::uint8_t allowed;
  if((1u<<(r8(m,body+0xaau)&31u))&2u)allowed=1;
  else if(seat_refuses)allowed=0;
  else allowed=weapon_animation_available(m,unit,key,r32(m,definition+0x288u),r32(m,definition+0x28cu));
  if(r32(m,body+0x13cu)!=none && (r32(m,definition+0x12cu)>>29)&1u)allowed=0;
  return allowed;
}

std::uint8_t weapon_slot_flagged(const M& m,std::uint32_t unit,std::uint32_t index) {
  const u32 weapon=weapon_in_slot(m,object_body(m,unit),0x212u+index);
  if(weapon==none)return 0;
  const std::uint8_t flags=r8(m,object_body(m,weapon)+0x16cu);
  return (flags>>1)&1u && !((flags>>2)&1u);
}

void weapon_pickup_interrupt(M& m,std::uint32_t unit,const Fp&) {
  const u32 block=action_block(m,object_body(m,unit));
  held_object_release(m,unit);
  w8(m,block+0xcu,0);
}

std::uint8_t weapon_select(M& m,std::uint32_t unit,std::uint8_t flag4,std::uint8_t flag5,bool primary,const Fp&) {
  const u32 weapon=weapon_in_slot(m,object_body(m,unit),primary?0x212u:0x213u);
  if(weapon==none)return 1;
  const u32 key=primary?0x8000025u:0xd000539u;
  if(unit_animation_state_active(m,unit,key) && !flag4)return 0;
  if(!(flag5 && flag4)) {
    if(weapon_lower(m,weapon,flag4)) {
      unit_weapon_channel_clear(m,unit);
      if(!flag5)unit_weapon_event(m,unit,primary?0x12u:0x13u);
      if(unit_weapon_state_start(m,unit,primary?0u:3u,key)) {
        if(!flag4)return 0;
        weapon_holster(m,unit,primary);
        return flag4;
      }
    } else {
      weapon_holster(m,unit,primary);
      return 1;
    }
  }
  weapon_holster(m,unit,primary);
  return 1;
}

void weapon_ready_animation(M& m,std::uint32_t unit,bool primary,std::uint32_t key,std::uint8_t flag0,
    std::uint8_t flag1,const Fp&) {
  const u32 k=primary?0u:1u;
  const u32 body=object_body(m,unit);
  const bool other=weapon_in_slot(m,body,0x216u+(primary?1u:0u))!=none;
  const bool vehicle=((1u<<(r8(m,body+0xaau)&31u))>>1)&1u;
  const auto slot=std::int8_t(r8(m,body+k+0x216u));
  if(slot==-1)return weapon_unready(m,unit,primary,0xffu);
  for(u32 n=0;n<2;++n)
    if(std::int8_t(r8(m,body+n+0x212u))==slot)return weapon_unready(m,unit,primary,std::uint8_t(slot));
  const u32 weapon=r32(m,body+0x218u+u32(i32(slot))*4u);
  if(weapon==none)return weapon_unready(m,unit,primary,std::uint8_t(slot));
  const u32 definition=tag_data(m,r32(m,object_body(m,weapon)));
  const u32 class0=other?0x400054bu:r32(m,definition+0x288u);
  if(weapon_animation_available(m,unit,key,class0,r32(m,definition+0x28cu)) || vehicle)
    weapon_ready_begin(m,unit,primary,class0,r32(m,definition+0x28cu));
  else if(r16(m,body+0x1fcu)==0xffffu)return weapon_unready(m,unit,primary,0);
  weapon_switch_effects(m,unit,k,0);
  w8(m,body+k+0x212u,std::uint8_t(slot));
  w32(m,body+u32(i32(slot))*4u+0x228u,r32(m,r32(m,0x51df54u)+8u));
  const std::uint8_t flagged=weapon_slot_flagged(m,unit,primary?1u:0u);
  weapon_ready_state(m,weapon,flag1,flag0);
  if(!flag1)unit_weapon_event(m,unit,flagged?0x16u:primary?0x14u:0x15u);
  unit_weapon_channel_clear(m,unit);
  w32(m,body+0x1c8u,none);
  w32(m,body+0x1ccu,none);
  w32(m,body+0x1d0u,none);
  w32(m,body+0x1e4u,0);
  w32(m,body+0x1e8u,0);
  w16(m,body+0x1e0u,0);
  if(flag0)return;
  if(other && primary)return;
  if(flagged && unit_weapon_state_start(m,unit,primary?0u:3u,primary?0x1000006eu:0x15000538u))return;
  unit_weapon_state_start(m,unit,primary?0u:3u,primary?0x5000024u:0xa000537u);
}

std::uint8_t weapon_trigger_ready(const M& m,std::uint32_t weapon,std::uint32_t trigger) {
  const u32 body=object_body(m,weapon);
  const i32 count=s32(r32(m,tag_data(m,r32(m,body))+0x2c0u));
  for(i32 n=0;n<count;++n) {
    if(trigger!=none && trigger!=u32(n))continue;
    const i32 state=i32(std::int16_t(r16(m,body+0x224u+sx16(std::uint16_t(n))*16u)));
    if(state>0 && state<=6)return 1;
  }
  return 0;
}

std::uint8_t weapon_trigger_update(M& m,std::uint32_t unit,std::uint32_t type) {
  const u32 weapon=weapon_in_slot(m,object_body(m,unit),i32(type)>1?0x213u:0x212u);
  u32 trigger;
  switch(type){case 0:case 0xa:trigger=0;break;case 1:case 0xb:trigger=1;break;
    default:refuse("E9870","trigger update of an action outside 0/1/A/B");}
  return weapon==none?0:weapon_trigger_ready(m,weapon,trigger);
}

void weapon_trigger_interrupt(M& m,std::uint32_t unit,std::uint32_t type) {
  const u32 weapon=weapon_in_slot(m,object_body(m,unit),i32(type)>1?0x213u:0x212u);
  if(weapon!=none)refuse("106670","weapon trigger release (from E9910)");
}

void action_target_clear(M& m,std::uint32_t unit) {
  w32(m,action_block(m,object_body(m,unit))+0x2cu,none);
}

std::uint8_t seat_release(M& m,std::uint32_t unit,const Fp&) {
  const u32 body=object_body(m,unit);
  if(r32(m,body+0x14u)==none || r16(m,body+0x1fcu)==0xffffu)return 0;
  refuse("D0520","seated unit release of "+hex_name(unit)+" (from ED060)");
}

std::uint8_t vehicle_exit_start(M& m,std::uint32_t unit,const Request& r,const Fp& fp) {
  const u32 body=object_body(m,unit);
  const u32 block=action_block(m,body);
  if((r32(m,block+4u)>>29)&1u)return 0;
  if((r32(m,action_block(m,object_body(m,unit))+4u)>>31)&1u)return 0;
  if(r8(m,action_block(m,object_body(m,unit))+8u)&1u)return 0;
  for(const u32 type:{0x1du,0x1cu,0x1fu,0x20u})interrupt(m,unit,type,fp);
  w8(m,body+0x258u,0);
  if(!seat_release(m,unit,fp))return 0;
  w32(m,block+4u,r32(m,block+4u)|0x20000000u);
  const u32 flags=r32(m,block);
  w32(m,block,r.bytes[4]?flags|1u:flags&~1u);
  if(r32(m,body+0x12cu)!=none)refuse("1C4600","actor vehicle exit speech (from ED110)");
  return 1;
}

// ---- CECF0 ----
void seat_drivers_update(M& m,std::uint32_t unit,const Fp& fp) {
  const auto is_unit=[](std::uint8_t type){return ((1u<<(type&31u))&3u)!=0;};
  const u32 body=object_body(m,unit);
  const u32 definition=tag_data(m,r32(m,body));
  const u32 previous=r32(m,body+0x248u);
  w32(m,body+0x248u,none);
  w32(m,body+0x24cu,none);
  if((r32(m,definition+0xbcu)>>26)&1u) {
    const u32 parent=r32(m,body+0x14u);
    if(parent!=none && r32(m,body+0x12cu)==none && is_unit(r8(m,object_row(m,parent)+3u)))w32(m,body+0x24cu,parent);
  }
  const auto gunner_if_free=[&](u32 child) {
    const u32 gunner=r32(m,body+0x24cu);
    if(gunner==none || gunner==r32(m,body+0x248u))w32(m,body+0x24cu,child);
  };
  for(u32 pass=0;pass<2;++pass) {
    for(u32 child=r32(m,body+0x10u);child!=none;) {
      const u32 child_body=object_body(m,child);
      if(is_unit(r8(m,child_body+0xaau))) {
        const std::uint16_t seat=r16(m,child_body+0x1fcu);
        if(seat==0xffffu) {
          if((r32(m,tag_data(m,r32(m,child_body))+0xbcu)>>25)&1u)gunner_if_free(child);
        } else {
          const u32 seats=r32(m,definition+0x1ccu);
          const u32 row=seats+sx16(seat)*0xb0u;
          const u32 flags=r32(m,row);
          const bool alternate=(flags>>11)&1u;
          if(!alternate || (flags>>13)&1u) {
            bool driver=(flags>>2)&1u,gunner=(flags>>3)&1u;
            if((flags>>18)&1u && r32(m,child_body+0x13cu)!=none)gunner=true;
            bool assign;
            if(pass==0 && !alternate && !(r8(m,action_block(m,child_body)+8u)&1u))assign=true;
            else {
              const bool exiting=pass!=0 && alternate && (r32(m,action_block(m,child_body)+4u)>>31)&1u;
              if(alternate) {
                const std::uint16_t other=r16(m,row+0x3eu);
                if(other!=0xffffu) {
                  const u32 other_flags=r32(m,seats+sx16(other)*0xb0u);
                  if((other_flags>>2)&1u)driver=true;
                  if((other_flags>>3)&1u)gunner=true;
                }
              }
              assign=exiting;
            }
            if(assign) {
              if(driver && !((r32(m,body+0x134u)>>1)&1u) && r32(m,body+0x248u)==none) {
                w32(m,body+0x248u,child);
                if(gunner && r32(m,body+0x24cu)==none)w32(m,body+0x24cu,child);
              } else if(gunner)gunner_if_free(child);
            }
          }
        }
        seat_drivers_update(m,child,fp);
      }
      child=r32(m,object_body(m,child)+0xcu);
    }
  }
  if(previous!=r32(m,body+0x248u))refuse("1C49D0","AI driver change hook (from CECF0)");
}

// ---- Seat queries ----
std::uint32_t seat_occupant(const M& m,std::uint32_t object,std::uint16_t seat) {
  for(u32 child=r32(m,object_body(m,object)+0x10u);child!=none;) {
    const u32 body=object_body(m,child);
    if(((1u<<(r8(m,body+0xaau)&31u))&3u) && r16(m,body+0x1fcu)==seat)return child;
    child=r32(m,body+0xcu);
  }
  return none;
}

std::uint8_t seat_mapping_admits(const M& m,std::uint32_t mapping,std::uint32_t object,std::uint32_t seat) {
  const u32 tag=r32(m,object_body(m,object));
  if(mapping==none)return 1;
  const u32 count=mapping>>16;
  u32 row=r32(m,r32(m,0x4ecc20u)+0x22cu)+(mapping&0xffffu)*8u;
  for(u32 n=0;n<count;++n,row+=8u)
    if(r32(m,row)==tag)return (r32(m,row+u32(i32(seat)>>5)*4u+4u)&(1u<<(seat&31u)))!=0;
  return 0;
}

void object_seats(const M& m,std::uint32_t object,std::vector<Seat>& out,std::int16_t max) {
  const auto count=[&]{return std::int16_t(out.size());};
  if(count()>=max)return;
  const u32 row=object_row(m,object),body=r32(m,row+8u);
  if(((1u<<(r8(m,row+3u)&31u))&3u) && !((r8(m,body+0x10au)>>2)&1u)) {
    const u32 definition=tag_data(m,r32(m,body));
    for(i32 n=0;n<s32(r32(m,definition+0x1c8u));++n) {
      if(count()>=max)return;
      out.push_back({object,std::int16_t(n),r32(m,definition+0x1ccu)+u32(n)*0xb0u});
    }
  }
  if(count()>=max)return;
  for(u32 child=r32(m,body+0x10u);child!=none;) {
    const u32 child_body=object_body(m,child);
    if(((1u<<(r8(m,child_body+0xaau)&31u))&3u) && (r32(m,child_body+4u)>>26)&1u)object_seats(m,child,out,max);
    child=r32(m,child_body+0xcu);
  }
}

std::vector<Seat> mapped_seats(const M& m,std::uint32_t mapping,std::uint32_t object,std::uint32_t filter,
    std::uint32_t occupancy,std::int32_t max) {
  std::vector<Seat> seats,kept;
  object_seats(m,object,seats,0x40);
  for(const Seat& seat:seats) {
    if(i32(kept.size())>=max)break;
    if(!seat_mapping_admits(m,mapping,seat.object,u32(i32(seat.seat))))continue;
    if(filter<=3) {
      const u32 flags=r32(m,seat.definition);
      const bool bit2=(flags>>2)&1u,bit3=(flags>>3)&1u;
      if(!(filter==0?!bit2:filter==1?bit3:filter==2?!bit2&&!bit3:bit2))continue;
    }
    if(occupancy==1 && seat_occupant(m,seat.object,std::uint16_t(seat.seat))==none)continue;
    if(occupancy==2 && seat_occupant(m,seat.object,std::uint16_t(seat.seat))!=none)continue;
    kept.push_back(seat);
  }
  return kept;
}

// ---- Script cores ----
std::uint16_t vehicle_unload(M& m,std::uint32_t mapping,std::uint32_t vehicle,const Fp& fp) {
  std::uint16_t started=0;
  if(vehicle==none)return 0;
  for(const Seat& seat:mapped_seats(m,mapping,vehicle,4,1,0x40)) {
    if(seat.object==none)continue;
    const u32 rider=seat_occupant(m,seat.object,std::uint16_t(seat.seat));
    if(rider==none)continue;
    if(request(m,rider,Request::of(0x1du),fp))++started;
  }
  return started;
}

void unit_exit_vehicle(M& m,std::uint32_t unit,std::uint16_t mode,const Fp& fp) {
  if(unit==none)return;
  const u32 body=object_body(m,unit);
  if(r32(m,body+0x14u)==none || r16(m,body+0x1fcu)==0xffffu)return;
  switch(mode) {
  case 0:request_type(m,unit,0x1du,fp);break;
  case 1:request(m,unit,Request::of(0x20u,0),fp);break;
  case 2:request(m,unit,Request::of(0x20u,1),fp);break;
  case 3:request(m,unit,Request::of(0x1du,1),fp);break;
  default:break;
  }
}

void ai_vehicle_exit(M& m,std::uint32_t mapping,const ActorWalk& next_actor,const Fp& fp) {
  for(u32 actor=next_actor();actor;actor=next_actor()) {
    if(mapping!=none) {
      if(r32(m,actor+0x26cu)==none)continue;
      const u32 body=object_body(m,r32(m,actor+0x18u));
      const u32 vehicle=r32(m,body+0x14u);
      if(vehicle==none)continue;
      const std::uint16_t seat=r16(m,body+0x1fcu);
      if(seat==0xffffu)continue;
      if(!seat_mapping_admits(m,mapping,vehicle,sx16(seat)))continue;
    }
    if(r32(m,actor+0x26cu)==none)continue;
    const u32 unit=r32(m,actor+0x18u);
    if(unit==none)continue;
    request(m,unit,Request::of(0x1du),fp);
  }
}
}
