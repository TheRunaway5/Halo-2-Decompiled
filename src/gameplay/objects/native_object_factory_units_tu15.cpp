// Native unit, biped and vehicle type callbacks (track "units"). Each owner
// replicates its TU1.5 routine over the world bank; oracle cases are in
// tools/native_object_factory_cases_units.py, harness kernels in
// tests/native_object_factory_units_kernels.cpp.
#include "halo2/native_object_factory_units_tu15.hpp"
#include "halo2/native_object_factory_unitactions_tu15.hpp"
#include "halo2/native_object_factory_core_tu15.hpp"
#include "halo2/native_object_factory_tu15.hpp"
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "halo2/native_object_factory_deviceanim_tu15.hpp"
#include "halo2/native_object_factory_havok_tu15.hpp"
#include "halo2/native_object_factory_objtypes_tu15.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <algorithm>
#include <cmath>

namespace halo2::native_object_factory_tu15 {
using namespace detail;
namespace {
namespace sse=native_campaign_script_fp;
using u32=std::uint32_t;
using u16=std::uint16_t;
using u8=std::uint8_t;
using i32=std::int32_t;
// .rdata constants.
constexpr u32 one_bits=0x3f800000u;            // 46A0D0
constexpr u32 three_bits=0x40400000u;          // 46A0E4
constexpr u32 two_bits=0x40000000u;            // 46A11C
constexpr u32 fifteen_bits=0x41700000u;        // 46A108
constexpr u32 six_bits=0x40c00000u;            // 46A10C
constexpr u32 ten_bits=0x41200000u;            // 46A110
constexpr u32 ik_epsilon_bits=0x38d1b717u;     // 46A0EC = 1e-4
constexpr u32 fade_fast_bits=0x3e88b439u;      // 450170 = 0.267
constexpr u32 fade_slow_bits=0x3d88b439u;      // 46AA4C = 0.06675
constexpr u32 blend_slow_bits=0x3e08b439u;     // 46A1E0 = 0.1335
constexpr u32 any_name=0x30000d9u;             // the graph's "any" mode/class

u32 manager_of(const M& m,u32 body){return body+sx16(r16(m,body+0x12au));}
u32 aim_block_of(const M& m,u32 body){return body+sx16(r16(m,body+0x33eu));}
// COMISS a,b then JA: ordered and a > b, denormal operands as zero under DAZ.
bool above(float a,float b,u32 mxcsr) {
  return sse::sse_compared(bits(a),mxcsr)>sse::sse_compared(bits(b),mxcsr);
}
// CVTTSS2SI: truncation; NaN and out of range give the integer indefinite.
u32 truncate(float value,u32 mxcsr) {
  value=sse::sse_compared(bits(value),mxcsr);
  if(!(value>=-2147483648.0f && value<2147483648.0f))return 0x80000000u;
  return u32(std::int32_t(value));
}

// ---- Animation graph: track weapon's natives (weapon_animation) ----
namespace graph=weapon_animation;

// ---- Fade curves ----
// 110FC0 (XMM2 x): 3x^2 - 2x^3 clamped to [0, 1].
float smoothstep(float x,u32 mxcsr) {
  float square=sse::mulss(x,x,mxcsr);
  const float scaled=sse::mulss(square,f32(three_bits),mxcsr);
  float cube=sse::mulss(square,x,mxcsr);
  cube=sse::mulss(cube,f32(two_bits),mxcsr);
  const float value=sse::subss(scaled,cube,mxcsr);
  if(above(0.0f,value,mxcsr))return 0.0f;
  if(above(value,f32(one_bits),mxcsr))return f32(one_bits);
  return value;
}
// 1E1920 (XMM3 x): 10x^3 - 15x^4 + 6x^5 clamped to [0, 1].
float smootherstep(float x,u32 mxcsr) {
  float cube=sse::mulss(sse::mulss(x,x,mxcsr),x,mxcsr);
  float fourth=cube;
  cube=sse::mulss(cube,f32(ten_bits),mxcsr);
  fourth=sse::mulss(fourth,x,mxcsr);
  float fifth=fourth;
  fourth=sse::mulss(fourth,f32(fifteen_bits),mxcsr);
  fifth=sse::mulss(fifth,x,mxcsr);
  fifth=sse::mulss(fifth,f32(six_bits),mxcsr);
  float value=sse::subss(fifth,fourth,mxcsr);
  value=sse::addss(value,cube,mxcsr);
  if(above(0.0f,value,mxcsr))return 0.0f;
  if(!above(value,f32(one_bits),mxcsr))return value;
  return f32(one_bits);
}

// ---- IK point iteration ----
// The cursor (116340/116280 locals): +0 marker, +4 target marker, +8 word
// entry index, +A word stage, +C mode, +10 weapon class. StackCursor is the
// production form (the original's frame); MemoryCursor reads and stores it
// in the world with the original widths (the oracle's form).
struct StackCursor {
  u32 marker=none,target_marker=none,mode=none,weapon_class=none;
  u16 entry=0xffffu,step=0;
  u16 index(const M&)const{return entry;}
  u16 stage(const M&)const{return step;}
  void set_stage(M*,u16 value){step=value;}
  u32 first_key(const M&)const{return mode;}
  u32 second_key(const M&)const{return weapon_class;}
  void found(M*,u32 a,u32 b,u16 n,bool){marker=a;target_marker=b;entry=n;}
};
struct MemoryCursor {
  u32 at;
  u16 index(const M& m)const{return r16(m,at+8u);}
  u16 stage(const M& m)const{return r16(m,at+0xau);}
  void set_stage(M* m,u16 value){w16(*m,at+0xau,value);}
  u32 first_key(const M& m)const{return r32(m,at+0xcu);}
  u32 second_key(const M& m)const{return r32(m,at+0x10u);}
  void found(M* m,u32 a,u32 b,u16 n,bool look) {
    w32(*m,at,a);
    if(look){w16(*m,at+8u,n);w32(*m,at+4u,b);}
    else{w32(*m,at+4u,b);w16(*m,at+8u,n);}
  }
};
// 1E5250 (ESI cursor, EBX graph): the weapon channel's next IK point over
// four stages (mode, class), (mode, any), (any, class), (any, any). A stage
// advance from a failed search restarts the entry count; one from an
// exhausted row keeps it (the original's quirk). `out` is null for the
// stack cursor (no world stores).
template<class Cursor>
bool ik_next_weapon(const M& m,M* out,Cursor& c,u32 graph) {
  u32 count=sx16(c.index(m))+1u;
  for(;;) {
    u32 found=0;
    if(std::int16_t(c.stage(m))>=4)return false;
    do {
      const u16 stage=c.stage(m);
      if(stage<=3u) {
        const u32 first_key=stage<2u?c.first_key(m):any_name;
        const u32 second_key=(stage&1u)?any_name:c.second_key(m);
        const u32 row=tag_block_search(m,graph+0x34u,first_key,0x14u);
        if(row)found=tag_block_search(m,row+4u,second_key,0x14u);
      }
      if(found)break;
      c.set_stage(out,u16(stage+1u));
      count=0;
    } while(std::int16_t(c.stage(m))<4);
    if(!found)return false;
    if(s32(count)<s32(r32(m,found+0xcu))) {
      const u32 entry=r32(m,found+0x10u)+count*8u;
      c.found(out,r32(m,entry),r32(m,entry+4u),u16(count),false);
      return true;
    }
    c.set_stage(out,u16(c.stage(m)+1u));
  }
}
// 1E51D0 (ESI cursor, EBX graph): the look channel's next IK point over two
// stages, mode then any (entries of the first-level row).
template<class Cursor>
bool ik_next_look(const M& m,M* out,Cursor& c,u32 graph) {
  u32 count=sx16(c.index(m))+1u;
  for(;;) {
    u32 found=0;
    if(std::int16_t(c.stage(m))>=2)return false;
    do {
      const u16 stage=c.stage(m);
      const i32 which=s32(sx16(stage));
      if(which==0)found=tag_block_search(m,graph+0x34u,c.first_key(m),0x14u);
      else if(which==1)found=tag_block_search(m,graph+0x34u,any_name,0x14u);
      if(found)break;
      c.set_stage(out,u16(stage+1u));
      count=0;
    } while(std::int16_t(c.stage(m))<2);
    if(!found)return false;
    if(s32(count)<s32(r32(m,found+0xcu))) {
      const u32 entry=r32(m,found+0x10u)+count*8u;
      c.found(out,r32(m,entry),r32(m,entry+4u),u16(count),true);
      return true;
    }
    c.set_stage(out,u16(c.stage(m)+1u));
  }
}
// 116340/116280 shared tail: fade `fade` of the unit's aim block, IK
// against `target` when its weight reaches 1e-4 in magnitude.
void unit_fade_ik(const M& m,u32 unit,u32 target,u32 fade,bool weapon_channel,u32 mxcsr) {
  const u32 body=object_body(m,unit);
  const u32 aim=aim_block_of(m,body);
  const u32 manager=manager_of(m,body);
  if(r32(m,manager+0x68u)==none || r32(m,manager)==none || r16(m,manager+6u)==0xffffu)return;
  if(target==none || !r8(m,aim+fade+1u))return;
  const u32 weight=animation_fade_weight(m,aim+fade,mxcsr);
  // FLD; FABS; FLD 1e-4; FCOMIP; JA: x87 compares exactly (no DAZ).
  if(f32(ik_epsilon_bits)>std::fabs(f32(weight)))return;
  unit_ik_points(m,unit,target,weapon_channel,weight,manager,mxcsr);
}
}

bool unit_animation_mode_listed(const M& m,std::uint32_t unit) {
  const u32 body=object_body(m,unit);
  const u32 mode=r32(m,manager_of(m,body)+0x7cu);
  for(u32 at=0x473990u;at<0x4739a4u;at+=4u)if(r32(m,at)==mode)return true;
  return false;
}

bool unit_dual_wielding(const M& m,std::uint32_t unit) {
  const u32 body=object_body(m,unit);
  return r8(m,body+0x212u)!=0xffu && r8(m,body+0x213u)!=0xffu;
}

std::uint32_t unit_weapon(const M& m,std::uint32_t unit,std::int16_t index) {
  if(index==-1)return none;
  return r32(m,object_body(m,unit)+u32(s32(index))*4u+0x218u);
}

std::uint8_t unit_animation_allows(const M& m,std::uint32_t unit,std::uint32_t bit) {
  const u32 body=object_body(m,unit);
  const u32 manager=manager_of(m,body);
  const u32 aim=aim_block_of(m,body);
  u8 result=1;
  if(r16(m,manager+6u)!=0xffffu) {
    const u32 animation=graph::graph_animation(m,tag_data(m,r32(m,manager)),r32(m,manager+4u));
    if(animation && ((1u<<(bit&31u))&r16(m,animation+0x18u)))result=0;
  }
  if(r16(m,aim+0xa2u)!=0xffffu) {
    const u32 animation=graph::graph_animation(m,tag_data(m,r32(m,aim+0x9cu)),r32(m,aim+0xa0u));
    if(animation && ((1u<<(bit&31u))&r16(m,animation+0x18u)))return 0;
  }
  return result;
}

std::uint8_t unit_look_enabled(const M& m,std::uint32_t unit) {
  const u32 body=object_body(m,unit);
  if(r32(m,body+0x14u)==none || r16(m,body+0x1fcu)==0xffffu)return 0;
  u8 result=1;
  if(unit_animation_mode_listed(m,unit))result=0;
  else switch(r32(m,manager_of(m,body)+0x7cu)) {
    case 0xa000767u:case 0x600008cu:case 0x50000c3u:case 0x400004au:case 0x400076cu:case 0x400076du:
    case 0x5000768u:case 0x5000769u:case 0x500076au:case 0x500076bu:case 0x700076eu:case 0x80000c4u:
    case 0x800076fu:case 0xe00002au:case 0xc000043u:case 0xd00002bu:case 0xd000042u:case 0xe00003bu:
    case 0xe0000c3u:case 0xf00003au:
      result=0;break;
    default:break;
  }
  if(!unit_animation_allows(m,unit,2))return 0;
  return result;
}

std::uint8_t unit_aim_enabled(const M& m,std::uint32_t unit) {
  const u32 body=object_body(m,unit);
  const u32 manager=manager_of(m,body);
  u8 result=1;
  switch(r32(m,aim_block_of(m,body)+0xa4u)) {
    case 0xc000075u:case 0xa000040u:case 0x6000006u:case 0x6000007u:case none:case 0x500000au:
    case 0x9000004u:case 0x9000005u:case 0x9000008u:case 0x9000009u:case 0xb0005b2u:case 0xa0005b9u:
    case 0xa0005bbu:case 0xc000073u:case 0xe00067du:case 0xc0006cdu:case 0xc000077u:case 0xc0006b3u:
    case 0xe000038u:case 0xe000607u:case 0xe000608u:case 0xe000609u:case 0xe00060au:case 0x130005bcu:
    case 0x11000074u:case 0x11000076u:case 0x140005b3u:
      break;
    default:result=0;break;
  }
  if(unit_animation_mode_listed(m,unit))result=0;
  else switch(r32(m,manager+0x7cu)) {
    case 0xc00005bu:case 0xb000059u:case 0xb00005au:case 0x400004au:case 0x600008cu:case 0xa00003eu:
    case 0xc000043u:case 0xc000058u:case 0xe0000c2u:case 0xd00003fu:case 0xd000042u:case 0xe00003bu:
    case 0xf00003au:case 0x1000003cu:
      result=0;break;
    default:break;
  }
  if(!unit_animation_allows(m,unit,3))return 0;
  return result;
}

void animation_fade_start(M& m,std::uint32_t fade,std::uint8_t active,std::uint32_t seconds_bits,const UnitFp& fp) {
  const u32 rate=sx16(r16(m,r32(m,0x51df54u)+2u));
  const float scaled=sse::mulss(sse::cvtsi2ss(s32(rate),fp.mxcsr),f32(seconds_bits),fp.mxcsr);
  i32 ticks=s32(sse::fistp32(scaled,fp.control_word));
  if(ticks<1)ticks=1;
  else if(ticks>0x7f)ticks=0x7f;
  const i32 length=ticks-1;
  if(length<=0) {
    w8(m,fade+1u,0);w8(m,fade,0);w8(m,fade+3u,0);
    return;
  }
  const u8 current=r8(m,fade+1u);
  if(current && active==(r8(m,fade+3u)&1u) && length==s32(sx8(current)))return;
  u8 position;
  if(current) {
    float divisor=sse::cvtsi2ss(s32(sx8(current)),fp.mxcsr);
    if(!above(divisor,f32(one_bits),fp.mxcsr))divisor=f32(one_bits);
    float scaled_position=sse::divss(sse::cvtsi2ss(s32(sx8(r8(m,fade))),fp.mxcsr),divisor,fp.mxcsr);
    scaled_position=sse::mulss(scaled_position,sse::cvtsi2ss(length-1,fp.mxcsr),fp.mxcsr);
    position=u8(truncate(scaled_position,fp.mxcsr));
  } else position=active?u8(1):u8(length-1);
  w8(m,fade+1u,u8(length));
  w8(m,fade,position);
  w8(m,fade+3u,active!=0);
}

std::uint32_t animation_fade_weight(const M& m,std::uint32_t fade,std::uint32_t mxcsr) {
  const u8 length=r8(m,fade+1u);
  if(!length)return 0;
  float divisor=sse::cvtsi2ss(s32(sx8(length)),mxcsr);
  if(!above(divisor,f32(one_bits),mxcsr))divisor=f32(one_bits);
  const u32 curve=sx8(r8(m,fade+2u))-1u;
  float x=sse::divss(sse::cvtsi2ss(s32(sx8(r8(m,fade))),mxcsr),divisor,mxcsr);
  const float one_f=f32(one_bits);
  switch(curve) {
    case 0:x=smoothstep(x,mxcsr);break;
    case 1:x=smootherstep(x,mxcsr);break;
    case 2: {
      x=sse::subss(x,one_f,mxcsr);
      x=sse::subss(one_f,sse::mulss(x,x,mxcsr),mxcsr);
      break;
    }
    case 3: {
      const float rest=sse::subss(one_f,x,mxcsr);
      const float cube=sse::mulss(sse::mulss(rest,rest,mxcsr),rest,mxcsr);
      x=sse::subss(one_f,cube,mxcsr);
      break;
    }
    default:break;
  }
  if(above(0.0f,x,mxcsr))return 0;
  if(above(x,one_f,mxcsr))return one_bits;
  return bits(x);
}

std::uint32_t tag_block_search(const M& m,std::uint32_t block,std::uint32_t key,std::uint32_t stride) {
  return graph::tag_block_search(m,block,key,stride);
}

std::uint32_t animation_graph_find(const M& m,std::uint32_t graph,std::uint32_t name) {
  if(!graph)return none;
  u32 result=none;        // low word inheritance index, high word animation
  u32 inheritance=none;   // -1: the graph itself
  u32 offset=0xffffffe0u; // inherited row offset
  u32 current=graph;
  for(;;) {
    u16 found=u16(result>>16);
    if(found!=0xffffu)break;
    const i32 count=s32(r32(m,current+0x2cu));
    bool hit=false;
    for(i32 n=0;n<count;++n) {
      const u32 at=u32(n)*0x6cu;
      const u32 row=at==0xffffff94u?0u:r32(m,current+0x30u)+at;
      if(r32(m,row)==name) {
        result=(u32(u16(n))<<16)|u16(inheritance);
        hit=u16(n)!=0xffffu;
        break;
      }
    }
    if(hit)continue;
    inheritance+=1u;
    offset+=0x20u;
    if(s32(inheritance)>=s32(r32(m,graph+0x4cu)))return result;
    const u32 tag=r32(m,r32(m,graph+0x50u)+offset+4u);
    current=tag==none?0u:tag_data(m,tag);
    if(!current) {
      if(u16(result>>16)==0xffffu)return result;
      break;
    }
  }
  graph::graph_require_resident(m,graph,result);
  return result;
}

void unit_ik_points(const M& m,std::uint32_t unit,std::uint32_t target,bool weapon_channel,std::uint32_t weight_bits,
    std::uint32_t manager,std::uint32_t mxcsr) {
  if(unit==none || target==none)return;
  if(!above(f32(weight_bits),0.0f,mxcsr))return;
  const u32 graph=tag_data(m,r32(m,manager+0x68u));
  if(!graph || !r16(m,graph+0xcu))return;
  // 1D4140: REP STOSD zeroes the node mask (+14 of the 0x104-byte frame)
  // for the graph's nodes; past the frame (or a negative count) the original
  // overwrites its own return address.
  const i32 words=(s32(sx16(r16(m,graph+0xcu)))+0x1f)>>5;
  if(words<0 || words>0x40)refuse("116060","IK node mask beyond the original frame");
  StackCursor cursor;
  for(;;) {
    cursor.mode=r32(m,manager+0x70u);
    cursor.weapon_class=r32(m,manager+0x74u);
    const u32 points_graph=tag_data(m,r32(m,manager+0x68u));
    if(!(weapon_channel?ik_next_weapon(m,nullptr,cursor,points_graph):ik_next_look(m,nullptr,cursor,points_graph)))
      return;
    if(cursor.target_marker==none || !cursor.target_marker)continue;
    if(cursor.marker==none || !cursor.marker)continue;
    refuse("1E1390","unit IK solve between markers (BAD60)");
  }
}

void unit_look_ik(const M& m,std::uint32_t unit,std::uint32_t parent,std::uint32_t mxcsr) {
  unit_fade_ik(m,unit,parent,0x84u,false,mxcsr);
}

void unit_aim_ik(const M& m,std::uint32_t unit,std::uint32_t weapon,std::uint32_t mxcsr) {
  unit_fade_ik(m,unit,weapon,0x80u,true,mxcsr);
}

bool animation_ik_point_next(M& m,std::uint32_t cursor,std::uint32_t graph,bool weapon_channel) {
  MemoryCursor c{cursor};
  return weapon_channel?ik_next_weapon(m,&m,c,graph):ik_next_look(m,&m,c,graph);
}

void unit_nodes_changed(M& m,std::uint32_t unit,std::uint32_t count,std::uint32_t matrices,const UnitFp& fp) {
  (void)count;(void)matrices;
  const u32 rows=r32(m,r32(m,object_pool)+0x44u);
  const u32 row_offset=(unit&0xffffu)*12u;
  const u32 body=r32(m,rows+row_offset+8u);
  const u32 definition=tag_data(m,r32(m,body));
  const u32 aim=aim_block_of(m,body);
  const u32 manager=manager_of(m,body);
  if((r32(m,definition+0xbcu)>>11)&1u)return;
  if(r32(m,manager+0x68u)==none || r32(m,manager)==none || r16(m,manager+6u)==0xffffu)return;
  const u8 look=unit_look_enabled(m,unit);
  u8 aiming=unit_aim_enabled(m,unit);
  if(unit_dual_wielding(m,unit))aiming=0;
  animation_fade_start(m,aim+0x84u,look,look?fade_fast_bits:fade_slow_bits,fp);
  animation_fade_start(m,aim+0x80u,aiming,aiming?fade_fast_bits:fade_slow_bits,fp);
  unit_look_ik(m,unit,r32(m,body+0x14u),fp.mxcsr);
  if(r8(m,body+0x212u)==0xffu)return;
  if(r32(m,manager+0x74u)==none || r32(m,manager+0x78u)==none)return;
  const u32 current=r32(m,r32(m,r32(m,object_pool)+0x44u)+row_offset+8u);
  const u32 weapon=unit_weapon(m,unit,std::int16_t(std::int8_t(r8(m,current+0x212u))));
  unit_aim_ik(m,unit,weapon,fp.mxcsr);
  w8(m,aim,r8(m,aim)&0xfeu);
}

// ---- Biped ----
namespace {
// 1F86A0 (EAX state): ground physics defaults.
void biped_ground_physics_reset(M& m,u32 p) {
  const u32 forward=r32(m,0x474d04u);
  w32(m,p+0xcu,none);
  for(u32 k=0;k<3;++k)w32(m,p+4u*k,r32(m,forward+4u*k));
  const u32 up=r32(m,0x474d10u);
  w8(m,p+0x62u,0);
  for(u32 k=0;k<3;++k)w32(m,p+0x64u+4u*k,r32(m,up+4u*k));
  const u16 word=r16(m,0x48a0b0u);
  const u32 matrix=r32(m,0x474d30u);
  w16(m,p+0x60u,word);
  w32(m,p+0x58u,none);w32(m,p+0x5cu,none);w32(m,p+0x1cu,none);w32(m,p+0x20u,none);
  w32(m,p+0x70u,0);
  for(u32 k=0;k<13;++k)w32(m,p+0x24u+4u*k,r32(m,matrix+4u*k));   // REP MOVSD
}
// 1F4850 (EAX state): flying physics defaults.
void biped_flying_physics_reset(M& m,u32 p) {
  const u32 up=r32(m,0x474d10u);
  for(u32 k=0;k<3;++k)w32(m,p+4u*k,r32(m,up+4u*k));
  const u32 forward=r32(m,0x474d04u);
  for(const u32 at:{0xdu,0xcu,0xeu,0xfu,0x10u,0x11u,0x13u})w8(m,p+at,0);
  w32(m,p+0x14u,none);w32(m,p+0x18u,none);
  w32(m,p+0x50u,r32(m,forward));w32(m,p+0x54u,r32(m,forward+4u));
  w8(m,p+0x12u,0);
  w32(m,p+0x58u,r32(m,forward+8u));
  w32(m,p+0x60u,none);w32(m,p+0x64u,none);
  w32(m,p+0x68u,r32(m,forward));w32(m,p+0x6cu,r32(m,forward+4u));
  w32(m,p+0x5cu,none);
  w32(m,p+0x70u,r32(m,forward+8u));
}
}

// 1ED780 (EDI state, stack mode).
void biped_physics_mode_set(M& m,std::uint32_t state,std::uint8_t mode,const UnitFp& fp) {
  const u8 old=r8(m,state);
  if(old>5u)refuse("1ED780","biped physics mode outside the original's table");
  if(old>=4u)refuse("204610","biped physics mode teardown (ragdoll/dead)");
  w8(m,state,mode);
  switch(mode) {
    case 1:biped_ground_physics_reset(m,state+0x10u);break;
    case 2: {
      const u32 forward=r32(m,0x474d04u);
      w32(m,state+0x10u,0);
      for(u32 k=0;k<3;++k)w32(m,state+0x14u+4u*k,r32(m,forward+4u*k));
      break;
    }
    case 3:biped_flying_physics_reset(m,state+0x10u);break;
    case 4:case 5:refuse("2045B0","biped ragdoll/dead physics mode");
    default:refuse("1ED780","biped physics mode outside the original's table");
  }
  const u8 now=r8(m,state);
  if(old!=now && (old==3u || now==3u || old==4u || old==5u || now==4u || now==5u)) {
    auto control=fp;
    havok_component_state_flags(m,r32(m,state+4u),0,control); // 1ED838..1ED83E
  }
}

std::uint32_t biped_physics_mode(const M& m,std::uint32_t biped) {
  const u32 body=object_body(m,biped);
  if((r8(m,body+0x10au)>>2)&1u)return 3;
  return ((r32(m,tag_data(m,r32(m,body))+0x264u)>>4)&1u)?2u:1u;
}

void biped_physics_initialize(M& m,std::uint32_t biped,const UnitFp& fp) {
  switch(biped_physics_mode(m,biped)) {
    case 1:biped_physics_mode_set(m,object_body(m,biped)+0x3dcu,1,fp);break;
    case 2: {   // E7CA0
      const u32 body=object_body(m,biped);
      biped_physics_mode_set(m,body+0x3dcu,2,fp);
      w16(m,body+0x34cu,0xffffu);w8(m,body+0x34eu,0);w8(m,body+0x34fu,0);
      break;
    }
    default: {  // E7CE0
      biped_physics_mode_set(m,object_body(m,biped)+0x3dcu,3,fp);
      refuse("E65A0","biped flying physics state (E7CE0)");
    }
  }
}

void biped_validate(M& m,std::uint32_t request,std::uint32_t mxcsr) {
  const u32 definition=tag_data(m,r32(m,request));
  if(!(r8(m,definition+0x264u)&1u))return;
  if((r32(m,definition+0x264u)>>4)&1u)return;
  const float height=f32(r32(m,definition+0x270u));
  for(u32 k=0;k<3;++k) {
    const float raised=sse::mulss(f32(r32(m,request+0x34u+4u*k)),height,mxcsr);
    w32(m,request+0x1cu+4u*k,bits(sse::addss(raised,f32(r32(m,request+0x1cu+4u*k)),mxcsr)));
  }
}

void biped_initialize(M& m,std::uint32_t biped,const UnitFp& fp) {
  const u32 offset=(biped&0xffffu)*12u;
  const u32 body=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  const u32 definition=tag_data(m,r32(m,body));
  w16(m,body+0x348u,0);w8(m,body+0x34au,0);w8(m,body+0x34bu,0);w8(m,body+0x34eu,0);w8(m,body+0x34fu,0);
  w16(m,body+0x34cu,0xffffu);
  w32(m,body+0x38cu,none);w32(m,body+0x390u,none);w32(m,body+0x394u,none);
  for(const u32 at:{0x398u,0x399u,0x39au,0x39bu})w8(m,body+at,0);
  w8(m,body+0x39cu,0x7fu);w8(m,body+0x39du,0);w8(m,body+0x39eu,0);
  w32(m,body+0x2c4u,0);w32(m,body+0x3a0u,0);w32(m,body+0x3a4u,0);
  object_world_position(m,biped,body+0x368u,fp.mxcsr);
  const u32 current=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  for(const u32 at:{0x354u,0x358u,0x35cu,0x360u,0x364u,0x350u})w32(m,current+at,none);
  if((r32(m,definition+0x1f0u)>>1)&1u)w8(m,body+0x348u,r8(m,body+0x348u)|8u);
  else w8(m,body+0x348u,r8(m,body+0x348u)&0xf7u);
  w32(m,body+0x3d8u,none);w32(m,body+0x3e0u,biped);w8(m,body+0x3dcu,0);
  w32(m,body+0x3e4u,none);w32(m,body+0x3e8u,none);
  biped_physics_initialize(m,biped,fp);
}

bool biped_new(M& m,std::uint32_t biped,std::uint32_t request,std::uint32_t failed,const UnitFp& fp) {
  (void)request;(void)failed;
  biped_initialize(m,biped,fp);
  return true;
}

void biped_new_failed(M& m,std::uint32_t biped) {
  if(r8(m,object_body(m,biped)+0x34bu)!=1u)return;
  const u32 counter=r32(m,0x4f3218u)+0x1130u;
  w32(m,counter,r32(m,counter)-1u);
}

// ---- Unit new failed ----
void unit_drop_weapons(M& m,std::uint32_t unit) {
  const u32 body=object_body(m,unit);
  for(u32 slot=0;slot<4;++slot)
    if(r32(m,body+0x218u+4u*slot)!=none)refuse("D0950","unit weapon slot release (CF400)");
}

void unit_new_failed(M& m,std::uint32_t unit) {
  const u32 offset=(unit&0xffffu)*12u;
  const u32 body=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  w8(m,body+0x216u,0xffu);w8(m,body+0x217u,0xffu);w8(m,body+0x212u,0xffu);w8(m,body+0x213u,0xffu);
  unit_drop_weapons(m,unit);
  const u32 current=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  if(r32(m,current+0x238u)!=none)refuse("BA570","delete the unit's held object (+238)");
}

// ---- 111B10 leaves ----
std::uint32_t animation_blend_seconds(std::uint32_t state,std::uint32_t previous,std::uint8_t forced,std::uint8_t urgent,
    std::uint32_t minimum_bits,std::uint32_t mxcsr) {   // 115E90
  const float fast=f32(fade_fast_bits);
  float seconds=fast;
  if(state==0x9000020u || state==0x900001fu || state==0x90006b2u)seconds=f32(blend_slow_bits);
  if(forced || urgent)seconds=fast;
  const auto vehicle_state=[](u32 name){return name==0xa000014u || name==0x9000015u || name==0x9000016u || name==0xa000017u;};
  if(previous==0x400000cu){if(vehicle_state(state))seconds=fast;}
  else if(vehicle_state(previous) && (vehicle_state(state) || state==0x400000cu))seconds=fast;
  const float minimum=f32(minimum_bits);
  if(!above(seconds,minimum,mxcsr))seconds=minimum;
  return bits(seconds);
}

std::uint32_t animation_fade_remaining(const M& m,std::uint32_t fade,std::uint32_t mxcsr) {   // 1E16E0
  const u8 length=r8(m,fade+1u);
  if(!length)return 0;
  const u32 rate=r32(m,r32(m,0x51df54u)+4u);
  i32 ticks=s32(sx8(r8(m,fade)));
  if(r8(m,fade+3u)&1u)ticks=s32(sx8(length))-ticks;
  return bits(sse::mulss(sse::cvtsi2ss(ticks,mxcsr),f32(rate),mxcsr));
}

bool animation_interpolation_start(M& m,std::uint32_t count,std::uint32_t manager,std::uint32_t seconds_bits,
    std::uint32_t destination,std::uint32_t source,const UnitFp& fp) {   // 1D38A0
  if(s32(count)<=0)return false;
  const float seconds=f32(seconds_bits);
  if(!above(seconds,0.0f,fp.mxcsr))return false;
  const u32 fade=manager+0x64u;
  if(r8(m,manager+0x65u) && !(r8(m,fade+3u)&2u)) {
    const float remaining=f32(animation_fade_remaining(m,fade,fp.mxcsr));
    // COMISS seconds, remaining; JB: below or unordered.
    if(!(sse::sse_compared(bits(seconds),fp.mxcsr)>=sse::sse_compared(bits(remaining),fp.mxcsr)))return false;
  }
  w8(m,fade+1u,0);w8(m,fade,0);w8(m,fade+3u,0);
  const u32 bytes=count<<5;
  for(u32 n=0;n<bytes>>2;++n)w32(m,destination+4u*n,r32(m,source+4u*n));   // REP MOVSD
  animation_fade_start(m,fade,1,seconds_bits,fp);
  return true;
}

void object_interpolation_start(M& m,std::uint32_t object,std::uint32_t seconds_bits,const UnitFp& fp) {   // BC380
  const u32 body=object_body(m,object);
  if(r16(m,body+0x112u)==0xffffu)return;
  if(r32(m,tag_data(m,r32(m,body))+0x38u)==none)return;
  const u32 count=sx16(r16(m,body+0x110u))>>5;
  animation_interpolation_start(m,count,body+sx16(r16(m,body+0x12au)),seconds_bits,body+sx16(r16(m,body+0x10eu)),
      body+sx16(r16(m,body+0x112u)),fp);
  native_tu15_wake_object_ancestors(m,object);
}

// 1D34E0 (ESI channel): only an unset channel is native.
std::uint8_t animation_overlay_restart(const M& m,std::uint32_t channel) {
  if(r32(m,channel)==none)return 0;
  refuse("1D34E0","overlay animation restart of a set channel");
}

void unit_overlays_restart(const M& m,std::uint32_t unit) {   // 115E20
  const u32 aim=aim_block_of(m,object_body(m,unit));
  animation_overlay_restart(m,aim+0x9cu);
  animation_overlay_restart(m,aim+0xdcu);
  animation_overlay_restart(m,aim+0xbcu);
}

// ---- Unit new (C6440) ----
// ---- 111B10: unit animation state (1D5320 from track weapon, 1E3F30 from
// track deviceanim) ----
namespace {
using weapon_animation::FrameWord;
weapon_animation::Fp script_fp(const UnitFp& fp){return {fp.control_word,fp.mxcsr};}
// A 0x20-byte channel record on the original's stack (1D3C80's local): its
// bytes stay private, every other access reaches the world memory.
class FrameRecord final : public SourcePhysicsContactMemory {
public:
  static constexpr u32 at=0xffffffc0u;
  explicit FrameRecord(M& world):world_(world){}
  void read(u32 address,std::span<std::byte> output)const override {
    if(address>=at && address-at+output.size()<=bytes.size()) {
      std::copy_n(bytes.begin()+(address-at),output.size(),output.begin());return;
    }
    world_.read(address,output);
  }
  void write(u32 address,std::span<const std::byte> input)override {
    if(address>=at && address-at+input.size()<=bytes.size()) {
      std::copy(input.begin(),input.end(),bytes.begin()+(address-at));return;
    }
    world_.write(address,input);
  }
  std::array<std::byte,0x20> bytes{};
private:
  M& world_;
};
u32 resolve_key(const M& m,u32 manager,u32 key,u32 offset) {
  return key==0x7000101u || key==none?r32(m,manager+offset):key;
}
// 1E3F30 with the manager's graph and first key ([+70]).
u32 overlay_handle(const M& m,u32 manager,u32 key3,u32 k1,u32 k2) {
  FrameWord result;
  device_animation::overlay_lookup(m,tag_data(m,r32(m,manager+0x68u)),key3,r32(m,manager+0x70u),k1,k2,nullptr,nullptr,
      nullptr,result);
  return result();
}
// 1D37D0 (ESI manager, ECX/EDX keys, EDI name, stack record, flags).
bool manager_start_overlay(M& m,u32 manager,u32 k1,u32 k2,u32 name,u32 record,u32 flags,const UnitFp& fp) {
  k1=resolve_key(m,manager,k1,0x74u);
  k2=resolve_key(m,manager,k2,0x78u);
  if(r32(m,manager+0x68u)==none)return false;
  const u32 handle=overlay_handle(m,manager,name,k1,k2);
  if(u16(handle>>16)==0xffffu)return false;
  return weapon_animation::manager_start_channel(m,manager,flags,record,handle,name,0xff,0xff,1,script_fp(fp));
}
}
// 1D37D0 exported for track nodebuild's 16AD45 (the first-person +B8
// channel).
bool manager_start_named_overlay(M& m,std::uint32_t manager,std::uint32_t k1,std::uint32_t k2,std::uint32_t name,
    std::uint32_t record,std::uint32_t flags,const UnitFp& fp) {
  return manager_start_overlay(m,manager,k1,k2,name,record,flags,fp);
}
// 1D3C80 (EAX manager, EDX key, stack key, EDI name): the overlay handle a
// trial start on a private record accepts, else NONE. Exported (track
// nodebuild's F2CC0 uses it).
std::uint32_t manager_overlay_trial(M& m,std::uint32_t manager,std::uint32_t k1,std::uint32_t k2,std::uint32_t name,
    const UnitFp& fp) {
  if(r32(m,manager+0x68u)==none)return none;
  FrameRecord frame(m);
  auto put=[&](u32 at,u32 value,u32 size){for(u32 k=0;k<size;++k)frame.bytes[at+k]=std::byte(value>>(8u*k));};
  put(0,none,4);put(4,0xffff,2);put(6,0xffff,2);put(8,none,4);put(0xe,0xffff,2);put(0xc,0xff,1);put(0xd,0xff,1);
  put(0x1c,0,4);put(0x10,0,1);put(0x11,0,1);put(0x12,0,2);put(0x14,0,2);put(0x16,0,2);put(0x18,one_bits,4);
  if(!manager_start_overlay(frame,manager,k1,k2,name,FrameRecord::at,0,fp))return none;
  return r32(frame,FrameRecord::at+4u);
}
namespace {
// 1D3C20 (EAX manager, EDX name, stack keys): 1D46A0's state animation.
u32 manager_state_handle(const M& m,u32 manager,u32 name,u32 k1,u32 k2) {
  FrameWord selected[4],resolved[4],result;
  if(!weapon_animation::choose_animation(m,manager,selected,resolved,r32(m,manager+0x70u),k1,k2,name,0,result))
    return none;
  return result();
}
// 1D3D80 (EAX name, ECX key, stack manager, out, key).
u32 manager_named_handle(M& m,u32 manager,u32 name,u32 k1,u32 k2,const UnitFp& fp) {
  k1=resolve_key(m,manager,k1,0x74u);
  k2=resolve_key(m,manager,k2,0x78u);
  const u32 handle=manager_overlay_trial(m,manager,k1,k2,name,fp);
  if(u16(handle>>16)!=0xffffu)return handle;
  return manager_state_handle(m,manager,name,k1,k2);
}
}
// 1D4450 (EAX manager, EDX aim channel 0..2). Exported (track nodebuild's
// F2CC0 uses it).
std::uint32_t manager_aim_handle(const M& m,std::uint32_t manager,std::uint32_t channel) {
  static constexpr u32 keys[3]={0xe000044u,0xe000045u,0xb000046u};
  if(channel>2u)return none;
  return overlay_handle(m,manager,keys[channel],r32(m,manager+0x74u),r32(m,manager+0x78u));
}
namespace {
// 1D4350 (ESI manager): any of the three aim overlays exists.
bool manager_has_aim_overlays(const M& m,u32 manager) {
  u32 handle=manager_aim_handle(m,manager,0);
  if(u16(handle>>16)==0xffffu) {
    handle=manager_aim_handle(m,manager,1);
    if(u16(handle>>16)==0xffffu)handle=manager_aim_handle(m,manager,2);
  }
  return u16(handle>>16)!=0xffffu;
}
// 115AF0's mode classes: (name, second name) for the first two handles.
std::pair<u32,u32> unit_mode_names(u32 mode,u32 parent) {
  constexpr std::pair<u32,u32> walk{0xc000026u,0xe000028u},stand{0xb000027u,0xd000029u},neither{none,none};
  switch(mode) {
    case 0x400000cu:case 0x900000eu:case 0x700005cu:case 0x700005du:case 0x800001eu:case 0x90006b2u:
    case 0x900001fu:case 0x9000020u:case 0xa00000fu:case 0xb000059u:case 0xb00005au:case 0xc00005bu:
    case 0xc000058u:
      return walk;
    case 0x5000049u:case 0x400004au:return parent==none?walk:neither;
    case 0xa000017u:case 0xa000014u:case 0x9000015u:case 0x9000016u:case 0xb000018u:case 0xa000019u:
    case 0xa00001au:case 0xa00002du:case 0xb00001bu:case 0xb00002eu:case 0xc000033u:case 0xc000034u:
    case 0xd000032u:case 0xd000035u:
      return stand;
    default:return neither;
  }
}
}

void unit_aim_overlay_start(M& m,std::uint32_t unit,const UnitFp& fp) {   // 116A10
  const u32 body=object_body(m,unit);
  const u32 manager=manager_of(m,body),aim=aim_block_of(m,body);
  const u32 k1=r32(m,manager+0x74u),k2=r32(m,manager+0x78u);
  if(r32(m,manager+0x68u)==none)return;
  const u32 handle=overlay_handle(m,manager,0x10000551u,k1,k2);
  if(u16(handle>>16)==0xffffu)return;
  if(weapon_animation::manager_start_channel(m,manager,0x803fu,aim+0xdcu,handle,0x10000551u,0xff,0xff,1,script_fp(fp)))
    w8(m,aim,r8(m,aim)&0x7fu);
}

void unit_animation_channels_refresh(M& m,std::uint32_t unit,const UnitFp& fp) {   // 115AF0
  const u32 body=object_body(m,unit);
  const u32 manager=manager_of(m,body),aim=aim_block_of(m,body);
  if(!manager)return;
  const u32 overlay=r32(m,aim+0x98u);
  const u32 k1=r32(m,manager+0x74u);
  const u32 mode=r32(m,manager+0x7cu);
  const u32 k2=r32(m,manager+0x78u);
  u32 parent=none;
  if(mode==0x5000049u || mode==0x400004au)parent=r32(m,body+0x14u);
  const auto [first,second]=unit_mode_names(mode,parent);
  w32(m,aim+0xfcu,manager_named_handle(m,manager,first,k1,k2,fp));
  w32(m,aim+0x100u,manager_named_handle(m,manager,second,k1,k2,fp));
  w32(m,aim+0x114u,manager_named_handle(m,manager,0x400004bu,k1,k2,fp));
  if(k1==0x400054bu) {
    w32(m,aim+0x104u,manager_named_handle(m,manager,0xb00054au,0x400054bu,k2,fp));
    w32(m,aim+0x108u,manager_named_handle(m,manager,0xb00054au,0x400054bu,overlay,fp));
    w32(m,aim+0x10cu,manager_named_handle(m,manager,0x400060bu,0x400054bu,k2,fp));
    w32(m,aim+0x110u,manager_named_handle(m,manager,0x400060bu,0x400054bu,overlay,fp));
  } else {
    for(u32 at=0x104u;at<0x10cu;at+=2u)w16(m,aim+at,0xffffu);
    w32(m,aim+0x10cu,manager_named_handle(m,manager,0x400060bu,k1,k2,fp));
    w32(m,aim+0x110u,manager_named_handle(m,manager,0x400060bu,k1,k2,fp));
  }
  unit_aim_overlay_start(m,unit,fp);
  bool any=false;
  for(u32 channel=0;channel<3;++channel) {
    w32(m,aim+0x118u+4u*channel,manager_aim_handle(m,manager,channel));
    if(r16(m,aim+0x11au+4u*channel)!=0xffffu)any=true;
  }
  w8(m,aim,any?u8(r8(m,aim)|2u):u8(r8(m,aim)&0xfdu));
}

bool unit_animation_state_set(M& m,std::uint32_t unit,std::uint32_t flags,const std::array<std::uint32_t,6>& state,
    const UnitFp& fp) {   // 111B10
  const u32 body=object_body(m,unit);
  const u32 manager=manager_of(m,body),aim=aim_block_of(m,body);
  const u32 previous=r32(m,manager+0x7cu);
  const u32 previous_mode=r32(m,manager+0x70u);
  const u32 key3=state[3],mode=state[0];
  if(previous!=none) {
    if(previous==key3 || key3==0x7000101u)flags|=0x20u;
    else {
      // The literal 0x474BE4 pairs of exchangeable states.
      static constexpr std::array<std::pair<u32,u32>,4> pairs{{{0xa000014u,0x9000015u},{0x9000015u,0xa000014u},
          {0x9000016u,0xa000017u},{0xa000017u,0x9000016u}}};
      u32 partner=none;
      for(const auto& [from,to]:pairs)if(key3==from){partner=to;break;}
      if(previous==partner)flags|=0x40u;
    }
  }
  if(r32(m,body+0x13cu)!=none && !r8(m,0x51daf8u))flags|=0x200u;
  u8 result=1;
  if(!(mode==0x7000101u && key3==mode)) {
    result=weapon_animation::manager_set_state(m,manager,key3,mode,state[1],state[2],flags,0x3f,script_fp(fp))?1:0;
    if(!result)return false;
  }
  const u8 urgent=state[5]&0xffu;
  const bool changed=(previous_mode!=mode && mode!=0x7000101u) || urgent;
  const bool restate=!((previous==key3 || key3==0x7000101u) && !urgent);
  if(changed) {
    unit_overlays_restart(m,unit);
    w8(m,aim,manager_has_aim_overlays(m,manager)?u8(r8(m,aim)|2u):u8(r8(m,aim)&0xfdu));
  } else if(!restate)return result;
  const u32 seconds=animation_blend_seconds(key3,previous,changed,0,state[4],fp.mxcsr);
  if(above(f32(seconds),0.0f,fp.mxcsr) && !((r8(m,manager+0x6cu)>>4)&1u))object_interpolation_start(m,unit,seconds,fp);
  unit_animation_channels_refresh(m,unit,fp);
  return result;
}

std::uint32_t model_node_index(const M& m,std::uint32_t model_tag,std::uint32_t name) {
  if(model_tag==none)return none;
  const u32 model=tag_data(m,model_tag);
  const i32 count=s32(r32(m,model+0x78u));
  if(count<=0)return none;
  u32 row=r32(m,model+0x7cu);
  for(i32 n=0;n<count;++n,row+=0x5cu)if(r32(m,row)==name)return u32(n);
  return none;
}

void unit_animation_state_clear(M& m,std::uint32_t unit) {   // 111720
  const u32 aim=aim_block_of(m,object_body(m,unit));
  w32(m,aim+0x98u,none);w32(m,aim+0x94u,none);w32(m,aim+0x9cu,none);
  w16(m,aim+0xa0u,0xffffu);w16(m,aim+0xa2u,0xffffu);
  w32(m,aim+0xb8u,0);w32(m,aim+0xb4u,one_bits);
  w32(m,aim+0xa4u,none);w8(m,aim+0xa8u,0xffu);w8(m,aim+0xa9u,0xffu);w16(m,aim+0xaau,0xffffu);
  w8(m,aim+0xacu,0);w8(m,aim+0xadu,0);w16(m,aim+0xaeu,0);w16(m,aim+0xb0u,0);w16(m,aim+0xb2u,0);
  w32(m,aim+0xbcu,none);w16(m,aim+0xc0u,0xffffu);w16(m,aim+0xc2u,0xffffu);
  w8(m,aim+0xccu,0);w8(m,aim+0xcdu,0);w16(m,aim+0xceu,0);w16(m,aim+0xd0u,0);w16(m,aim+0xd2u,0);
  w32(m,aim+0xd8u,0);w32(m,aim+0xd4u,one_bits);
  w32(m,aim+0xc4u,none);w8(m,aim+0xc8u,0xffu);w8(m,aim+0xc9u,0xffu);w16(m,aim+0xcau,0xffffu);
  w32(m,aim+0xdcu,none);w16(m,aim+0xe0u,0xffffu);w16(m,aim+0xe2u,0xffffu);
  w8(m,aim+0xecu,0);w8(m,aim+0xedu,0);w16(m,aim+0xeeu,0);w16(m,aim+0xf0u,0);w16(m,aim+0xf2u,0);
  w32(m,aim+0xf8u,0);w32(m,aim+0xf4u,one_bits);
  w32(m,aim+0xe4u,none);w8(m,aim+0xe8u,0xffu);w8(m,aim+0xe9u,0xffu);w16(m,aim+0xeau,0xffffu);
  w8(m,aim+3u,0);
  for(u32 k=0;k<7;++k){w16(m,aim+0xfcu+4u*k,0xffffu);w16(m,aim+0xfeu+4u*k,0xffffu);}
  for(u32 k=0;k<3;++k){w16(m,aim+0x118u+4u*k,0xffffu);w16(m,aim+0x11au+4u*k,0xffffu);}
  w8(m,aim,r8(m,aim)&0xfdu);
}

void unit_overlay_animations_find(M& m,std::uint32_t unit) {   // 111000
  const u32 body=object_body(m,unit);
  const u32 aim=aim_block_of(m,body),manager=manager_of(m,body);
  for(u32 k=0;k<8;++k)w32(m,aim+0x5cu+4u*k,0);
  w16(m,aim+0x6cu,0xffffu);
  const u32 id=animation_graph_find(m,tag_data(m,r32(m,manager+0x68u)),0x800004du);
  w32(m,aim+0x5cu,id);
}

void unit_aim_animations_find(M& m,std::uint32_t unit) {   // 1102A0
  const u32 body=object_body(m,unit);
  const u32 manager=manager_of(m,body),aim=aim_block_of(m,body);
  zero_fill(m,aim+0x30u,0x2cu);
  const u32 first=animation_graph_find(m,tag_data(m,r32(m,manager+0x68u)),0x8000145u);
  w32(m,aim+0x34u,first);
  const u32 second=animation_graph_find(m,tag_data(m,r32(m,manager+0x68u)),0xc00024du);
  w32(m,aim+0x40u,0);w32(m,aim+0x38u,none);w32(m,aim+0x30u,second);
}

void unit_fades_reset(M& m,std::uint32_t unit) {   // 115F50
  const u32 aim=aim_block_of(m,object_body(m,unit));
  for(const u32 at:{0x81u,0x80u,0x83u,0x85u,0x84u,0x87u,0x89u,0x88u,0x8bu,0x8du,0x8cu,0x8fu,0x91u,0x90u,0x93u})
    w8(m,aim+at,0);
  w8(m,aim+0x82u,1);w8(m,aim+0x86u,1);w8(m,aim+0x8au,2);w8(m,aim+0x8eu,1);w8(m,aim+0x92u,1);
}

void unit_animation_reset(M& m,std::uint32_t unit) {   // 111940
  const u32 body=object_body(m,unit);
  const u32 aim=aim_block_of(m,body);
  unit_animation_state_clear(m,unit);
  w32(m,aim+0x28u,none);
  w16(m,aim,0);
  for(u32 k=0;k<4;++k)w32(m,aim+4u+4u*k,0);
  for(u32 k=0;k<4;++k)w32(m,aim+0x14u+4u*k,0);
  w8(m,aim+2u,0);w8(m,aim+3u,0);
  const u32 definition=tag_data(m,r32(m,body));
  w16(m,aim+0x7cu,u16(model_node_index(m,r32(m,definition+0x38u),0x80001a2u)));
  w16(m,aim+0x7eu,u16(model_node_index(m,r32(m,definition+0x38u),0x90001a3u)));
  const u32 manager=manager_of(m,body);
  if(manager){w32(m,manager+0x74u,none);w32(m,manager+0x78u,none);}
  unit_overlay_animations_find(m,unit);
  unit_aim_animations_find(m,unit);
  unit_fades_reset(m,unit);
  w32(m,aim+0x94u,none);w32(m,aim+0x98u,none);
}

void unit_variant_record(M& m,std::uint32_t unit,std::uint32_t variant) {   // 1175A0
  const u32 offset=(unit&0xffffu)*12u;
  const u32 body=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  const u32 record=body+sx16(r16(m,body+0x342u));
  const u32 model=tag_data(m,r32(m,tag_data(m,r32(m,body))+0x38u));
  w32(m,record,r32(m,model+0x90u));
  w32(m,record+8u,r32(m,model+0xa0u));
  if(s32(r32(m,model+0x50u))<=0)return;
  for(u32 n=0;;) {
    const u32 row=r32(m,model+0x54u)+sx16(u16(n))*0x38u;
    const u32 dialogue=r32(m,row+0x34u);
    const bool exact=r32(m,row)==variant && dialogue!=none;
    if(exact || (dialogue!=none && r32(m,record)==none)) {
      const u32 current=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
      const u32 target=current+sx16(r16(m,current+0x342u));
      const u32 info=r32(m,tag_data(m,dialogue)+0x14u);
      w32(m,target,dialogue);w32(m,target+4u,info);
      const u32 override=r32(m,row+0x2cu);
      if(override && override!=none)w32(m,record+8u,override);
      if(exact)return;
    }
    ++n;
    if(s32(sx16(u16(n)))>=s32(r32(m,model+0x50u)))return;
  }
}

bool unit_new(M& m,std::uint32_t unit,std::uint32_t request,std::uint32_t failed,std::uint32_t scratch_slot,
    const UnitFp& fp,const UnitAnimationStateService& animation_state) {
  const auto state_set=[&](std::uint32_t flags,const std::array<std::uint32_t,6>& state) {
    return animation_state?animation_state(m,unit,flags,state):unit_animation_state_set(m,unit,flags,state,fp);
  };
  const u32 offset=(unit&0xffffu)*12u;
  const u32 body=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  const u32 definition=tag_data(m,r32(m,body));
  w32(m,body+0x238u,none);
  for(u32 k=0;k<4;++k)w32(m,body+0x218u+4u*k,none);
  w16(m,body+0x214u,0);w16(m,body+0x210u,0);
  for(const u32 at:{0x213u,0x212u,0x217u,0x216u,0x23cu,0x23du,0x240u,0x241u})w8(m,body+at,0xffu);
  for(const u32 at:{0x13cu,0x140u,0x12cu,0x130u})w32(m,body+at,none);
  w16(m,body+0x1fcu,0xffffu);
  for(const u32 at:{0x248u,0x24cu,0x2a0u})w32(m,body+at,none);
  w16(m,body+0x2a4u,0xffffu);
  w32(m,body+0x2a8u,none);w32(m,body+0x2acu,none);
  w32(m,body+0x1ecu,0);w16(m,body+0x2c8u,0);w16(m,body+0x2cau,0);w32(m,body+0x2ccu,0);
  for(const u32 at:{0x2d0u,0x2e0u,0x250u,0x254u})w32(m,body+at,none);
  w8(m,body+0x258u,0);w16(m,body+0x25au,0xffffu);w8(m,body+0x1f4u,0);w32(m,body+0x2d4u,none);
  w16(m,body+0x2eau,0);w32(m,body+0x2ecu,none);w8(m,body+0x1f6u,0xffu);w8(m,body+0x1f7u,0xffu);
  for(const u32 at:{0x334u,0x338u,0x1e4u,0x1e8u})w32(m,body+at,0);
  for(const u32 at:{0x1c8u,0x1ccu,0x1d0u})w32(m,body+at,none);
  w16(m,body+0x1e0u,0);
  for(u32 k=0;k<4;++k) {
    w32(m,body+0x2f0u+0x10u*k,none);w32(m,body+0x2f4u+0x10u*k,0);
    w32(m,body+0x2f8u+0x10u*k,none);w32(m,body+0x2fcu+0x10u*k,none);
    w8(m,body+0x330u+k,0);
  }
  const u32 rows=r32(m,r32(m,object_pool)+0x44u);
  if(r16(m,r32(m,rows+offset+8u)+0x12au)==0xffffu)return false;
  if(!object_block_allocate(m,unit,0x33c,0x124,0,scratch_slot) || !object_block_allocate(m,unit,0x340,0x58,0,scratch_slot)
     || !object_block_allocate(m,unit,0x344,0x44,0,scratch_slot)) {
    w8(m,failed,1);
    return false;
  }
  const u32 current=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  const u32 extension=current+sx16(r16(m,current+0x346u));
  w32(m,extension+0x10u,none);w16(m,extension+0x14u,0xffffu);w32(m,extension+0x18u,none);w32(m,extension+0x2cu,none);
  w32(m,current+sx16(r16(m,current+0x33eu))+0x2cu,none);
  unit_animation_reset(m,unit);
  const u32 record=current+sx16(r16(m,current+0x342u));
  w32(m,record,none);w32(m,record+0x44u,none);
  unit_variant_record(m,unit,r32(m,request+0xcu));
  for(const u32 at:{0x18cu,0x180u,0x168u,0x15cu,0x150u})
    for(u32 k=0;k<3;++k)w32(m,current+at+4u*k,r32(m,current+0x70u+4u*k));
  w32(m,current+0x14cu,0x6000086u);
  const u16 seat=r16(m,definition+0x1b4u);
  if(std::int16_t(seat)>=0 && std::int16_t(seat)<2 && std::int16_t(r16(m,definition+0x1b6u))>=0)
    w8(m,current+sx16(seat)+0x23eu,r8(m,definition+0x1b6u));
  w32(m,current+4u,r32(m,current+4u)|0x1000u);
  const u16 team=r16(m,request+0x64u);
  w16(m,current+0x138u,team);
  if(r32(m,r32(m,0x4f3218u)+8u)==1u && (team==0 || team==0xffffu))w16(m,current+0x138u,r16(m,definition+0xc0u));
  state_set(2,{0x7000001u,0x7000101u,0x7000101u,0x7000001u,0,0});
  // C67B2..: the model's 0x80001A2 and 0x90001A3 marker rows.
  w8(m,current+0x1f6u,u8(model_node_index(m,r32(m,definition+0x38u),0x80001a2u)));
  w8(m,current+0x1f7u,u8(model_node_index(m,r32(m,definition+0x38u),0x90001a3u)));
  return true;
}

// ---- Vehicle new (F10E0) ----
// 1CE7B0 (ESI channel): restart the channel's animation at its last frame.
void animation_channel_restart(M& m,std::uint32_t channel,std::uint32_t mxcsr) {
  u32 animation=0;
  if(r16(m,channel+6u)!=0xffffu)animation=graph::graph_animation(m,tag_data(m,r32(m,channel)),r32(m,channel+4u));
  float frame=sse::cvtsi2ss(s32(sx16(r16(m,animation+0x14u)))-1,mxcsr);
  frame=sse::addss(frame,f32(ik_epsilon_bits),mxcsr);
  if(above(0.0f,frame,mxcsr))frame=0.0f;
  const u8 flags=u8(r8(m,channel+0x11u)|0xau);
  w32(m,channel+0x1cu,bits(frame));
  w8(m,channel+0x11u,flags);
  w8(m,channel+0x10u,0);
}

// 1D6200 (EDI manager): vehicle animation manager defaults after its state.
void vehicle_animation_settle(M& m,std::uint32_t manager,std::uint32_t mxcsr) {
  w32(m,manager+0x40u,none);w16(m,manager+0x44u,0xffffu);w16(m,manager+0x46u,0xffffu);
  w32(m,manager+0x5cu,0);
  w8(m,manager+0x50u,0);w8(m,manager+0x51u,0);w16(m,manager+0x52u,0);
  w32(m,manager+0x58u,one_bits);
  w16(m,manager+0x54u,0);w16(m,manager+0x56u,0);
  w32(m,manager+0x48u,none);w8(m,manager+0x4cu,0xffu);w8(m,manager+0x4du,0xffu);w16(m,manager+0x4eu,0xffffu);
  if(r32(m,manager)!=none && r16(m,manager+6u)!=0xffffu)animation_channel_restart(m,manager,mxcsr);
  const u32 overlay=manager+0x20u;
  if(r32(m,overlay)!=none && r16(m,overlay+6u)!=0xffffu)animation_channel_restart(m,overlay,mxcsr);
  w8(m,manager+0x65u,0);w8(m,manager+0x64u,0);w8(m,manager+0x67u,0);
  if(r32(m,manager+0x40u)!=none && r16(m,manager+0x46u)!=0xffffu && (r8(m,manager+0x52u)&1u))
    w8(m,manager+0x51u,r8(m,manager+0x51u)|1u);
  if(r32(m,manager)!=none && r16(m,manager+6u)!=0xffffu && (r8(m,manager+0x12u)&1u))
    w8(m,manager+0x11u,r8(m,manager+0x11u)|1u);
  if(r32(m,overlay)!=none && r16(m,overlay+6u)!=0xffffu && (r8(m,overlay+0x12u)&1u))
    w8(m,overlay+0x11u,r8(m,overlay+0x11u)|1u);
}


std::uint32_t object_root_origin(M& m,std::uint32_t object,std::uint32_t out,std::uint32_t mxcsr) {   // BBF20
  const u32 rows=r32(m,r32(m,object_pool)+0x44u);
  u32 root=none;
  if(object!=none)for(u32 at=object;at!=none;at=r32(m,r32(m,rows+(at&0xffffu)*12u+8u)+0x14u))root=at;
  const u32 component=r32(m,r32(m,rows+(root&0xffffu)*12u+8u)+0xb4u);
  if(component!=none) {
    const u32 record=r32(m,r32(m,0x52be5cu)+0x44u)+(component&0xffffu)*0xa0u;
    const u8 body=r8(m,record+0x18u);
    const u32 index=std::int8_t(body)>=0 && s32(sx8(body))<s32(r32(m,record+0x74u))?sx8(body):none;
    if(index!=none) {
      const u32 rigid=r32(m,r32(m,record+0x70u)+index*0x60u+0x40u);
      const u32 motion=r32(m,rigid+0x3cu);
      const u32 z=r32(m,motion+0x78u),y=r32(m,motion+0x74u);
      w32(m,out,r32(m,motion+0x70u));w32(m,out+4u,y);w32(m,out+8u,z);
      return out;
    }
  }
  object_world_position(m,object,out,mxcsr);
  return out;
}

void vehicle_state_initialize(M& m,std::uint32_t vehicle,std::uint32_t mxcsr) {   // F0F10
  const u32 body=object_body(m,vehicle);
  w8(m,body+0x34au,0);w16(m,body+0x348u,0);
  for(const u32 at:{0x34bu,0x34cu,0x34du,0x34eu,0x34fu,0x350u,0x384u,0x385u,0x386u,0x387u,0x388u})w8(m,body+at,0);
  w16(m,body+0x38eu,0);w8(m,body+0x351u,0xffu);
  for(const u32 at:{0x354u,0x358u,0x35cu,0x334u,0x338u,0x360u,0x364u,0x368u,0x36cu,0x374u,0x370u,0x378u,0x37cu})w32(m,body+at,0);
  w8(m,body+0x38cu,0xfeu);w32(m,body+0x380u,0);w8(m,body+0x38bu,0xffu);
  w32(m,body+0x398u,0);w32(m,body+0x39cu,0);w32(m,body+0x390u,0);w32(m,body+0x394u,0);w32(m,body+0x3acu,0);
  w8(m,body+0x349u,r8(m,body+0x349u)|1u);
  object_root_origin(m,vehicle,body+0x3d0u,mxcsr);
  for(const u32 at:{0x3b8u,0x3bcu,0x3c8u,0x3c0u,0x3ccu,0x400u})w32(m,body+at,none);
  for(u32 at=0x404u;at<0x428u;at+=2u)w16(m,body+at,0xffffu);
}

bool vehicle_new(M& m,std::uint32_t vehicle,std::uint32_t request,std::uint32_t failed,const UnitFp& fp,
    const UnitAnimationStateService& animation_state) {
  (void)request;(void)failed;
  const auto state_set=[&](std::uint32_t flags,const std::array<std::uint32_t,6>& state) {
    return animation_state?animation_state(m,vehicle,flags,state):unit_animation_state_set(m,vehicle,flags,state,fp);
  };
  const u32 mxcsr=fp.mxcsr;
  const u32 offset=(vehicle&0xffffu)*12u;
  const u32 body=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  const u32 definition=tag_data(m,r32(m,body));
  if(r32(m,tag_data(m,r32(m,definition+0x38u))+0x24u)==none)return false;
  w32(m,body+0x3a0u,none);w32(m,body+0x3a4u,none);w32(m,body+0x3a8u,0);w32(m,body+0x3b0u,0);w32(m,body+0x3b4u,0);
  vehicle_state_initialize(m,vehicle,mxcsr);
  if(r8(m,definition+0x2acu)&1u)object_wake(m,vehicle,true,fp); // F1164: DL=1 (deferred)
  if(r16(m,r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u)+0x12au)==0xffffu)return true;
  if(state_set(0,{0x7000001u,0x7000101u,0x7000101u,0x700005cu,0,0})) {
    vehicle_animation_settle(m,body+sx16(r16(m,body+0x12au)),mxcsr);
    return true;
  }
  state_set(0,{0x7000001u,0x7000101u,0x7000101u,0x400000cu,0,0});
  return true;
}

// ---- Unit place (C6850) ----
void unit_create_starting_weapons(M& m,std::uint32_t unit) {
  const u32 definition=tag_data(m,r32(m,object_body(m,unit)));
  for(i32 n=0;n<s32(r32(m,definition+0x1c0u));++n) {
    const u32 weapon=r32(m,r32(m,definition+0x1c4u)+u32(n)*8u+4u);
    if(weapon==none)continue;
    refuse("B9B70","unit starting weapon birth (B9960 request, then object_new: coordinator)");
  }
}

void unit_place(M& m,std::uint32_t unit,const UnitFp& fp) {
  unit_create_starting_weapons(m,unit);
  // C6860..C6890: E8E50 (EAX unit, EDI a zeroed stack request of type 8):
  // the unitactions track's dispatcher, which performs its own B9390 wake,
  // then action 8's start EAF00 and the player action history.
  unit_actions::request(m,unit,unit_actions::Request::of(8),fp);
  // C6896: CECF0, the seated drivers and gunners.
  unit_actions::seat_drivers_update(m,unit,fp);
}
}
