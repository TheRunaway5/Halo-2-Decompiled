// Native object factory coordinator owners (track "core"). Each replicates
// its TU1.5 routine over the world bank; oracle cases are in
// tools/native_object_factory_cases_core.py.
#include "halo2/native_object_factory_core_tu15.hpp"
#include "halo2/native_object_factory_tu15.hpp"
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "halo2/native_tu15_object_spatial_packet.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "native_object_factory_tu15_internal.hpp"

namespace halo2::native_object_factory_tu15 {
using namespace detail;
namespace {
// 0x474B90: per type, its chain levels (+84 list, object first) and every
// non-zero slot (+10..+80) of each level. Literal TU1.5 .data generated from
// the authenticated XBE; tools/native_object_factory_cases_core.py rechecks
// this block against the XBE whenever the oracle loads.
struct Entry {std::uint8_t type,level,slot;std::uint32_t value;};
// BEGIN 0x474B90
constexpr std::array<std::array<std::uint32_t,3>,13> chain_levels{{
  {0x473e48u,0x473f10u,0x473fd8u},
  {0x473e48u,0x473f10u,0x4740a0u},
  {0x473e48u,0x474168u,0x474230u},
  {0x473e48u,0x474168u,0x4742f8u},
  {0x473e48u,0x474168u,0x4743c0u},
  {0x473e48u,0x474488u,0x0u},
  {0x473e48u,0x474550u,0x0u},
  {0x473e48u,0x4747a8u,0x474870u},
  {0x473e48u,0x4747a8u,0x474938u},
  {0x473e48u,0x4747a8u,0x474a00u},
  {0x473e48u,0x474618u,0x0u},
  {0x473e48u,0x4746e0u,0x0u},
  {0x473e48u,0x474ac8u,0x0u},
}};
constexpr Entry entries[]={
  {0,0,0x38,0xc0790u},{0,0,0x4c,0xbec50u},{0,0,0x80,0x2e4d60u},{0,1,0x2c,0xc6440u},{0,1,0x38,0xc6850u},
  {0,1,0x3c,0xc68b0u},{0,1,0x40,0xc9010u},{0,1,0x4c,0xc9070u},{0,1,0x54,0xd2ec0u},{0,1,0x58,0xd1220u},
  {0,1,0x5c,0xd1390u},{0,1,0x60,0x2e4ea0u},{0,1,0x6c,0x114750u},{0,1,0x70,0x1154a0u},{0,1,0x74,0x1157d0u},
  {0,1,0x7c,0x116df0u},{0,1,0x80,0x2e4d60u},{0,2,0x10,0x31d400u},{0,2,0x14,0x31d400u},{0,2,0x18,0x31d400u},
  {0,2,0x1c,0x31d400u},{0,2,0x20,0x2e4d60u},{0,2,0x24,0xde970u},{0,2,0x28,0xde9e0u},{0,2,0x2c,0xdea60u},
  {0,2,0x30,0xde9c0u},{0,2,0x3c,0xdea70u},{0,2,0x40,0xdf880u},{0,2,0x44,0xdfee0u},{0,2,0x4c,0xe0900u},
  {0,2,0x58,0xdeaa0u},{0,2,0x68,0xe24f0u},{0,2,0x78,0xe0a10u},{0,2,0x80,0x2e4d60u},{1,0,0x38,0xc0790u},
  {1,0,0x4c,0xbec50u},{1,0,0x80,0x2e4d60u},{1,1,0x2c,0xc6440u},{1,1,0x38,0xc6850u},{1,1,0x3c,0xc68b0u},
  {1,1,0x40,0xc9010u},{1,1,0x4c,0xc9070u},{1,1,0x54,0xd2ec0u},{1,1,0x58,0xd1220u},{1,1,0x5c,0xd1390u},
  {1,1,0x60,0x2e4ea0u},{1,1,0x6c,0x114750u},{1,1,0x70,0x1154a0u},{1,1,0x74,0x1157d0u},{1,1,0x7c,0x116df0u},
  {1,1,0x80,0x2e4d60u},{1,2,0x10,0x31d400u},{1,2,0x14,0x31d400u},{1,2,0x18,0x31d400u},{1,2,0x1c,0x31d400u},
  {1,2,0x2c,0xf10e0u},{1,2,0x30,0xf0e70u},{1,2,0x40,0xf2340u},{1,2,0x4c,0xf9270u},{1,2,0x6c,0xf34b0u},
  {1,2,0x78,0xf0f10u},{1,2,0x80,0x2e4d60u},{2,0,0x38,0xc0790u},{2,0,0x4c,0xbec50u},{2,0,0x80,0x2e4d60u},
  {2,1,0x10,0x31d400u},{2,1,0x14,0x31d400u},{2,1,0x18,0x31d400u},{2,1,0x1c,0x31d400u},{2,1,0x20,0xfa6f0u},
  {2,1,0x2c,0xfa820u},{2,1,0x40,0xfaa30u},{2,2,0x2c,0x102ad0u},{2,2,0x30,0x102a00u},{2,2,0x40,0x1030f0u},
  {2,2,0x4c,0x1046c0u},{3,0,0x38,0xc0790u},{3,0,0x4c,0xbec50u},{3,0,0x80,0x2e4d60u},{3,1,0x10,0x31d400u},
  {3,1,0x14,0x31d400u},{3,1,0x18,0x31d400u},{3,1,0x1c,0x31d400u},{3,1,0x20,0xfa6f0u},{3,1,0x2c,0xfa820u},
  {3,1,0x40,0xfaa30u},{3,2,0x30,0xfa5f0u},{4,0,0x38,0xc0790u},{4,0,0x4c,0xbec50u},{4,0,0x80,0x2e4d60u},
  {4,1,0x10,0x31d400u},{4,1,0x14,0x31d400u},{4,1,0x18,0x31d400u},{4,1,0x1c,0x31d400u},{4,1,0x20,0xfa6f0u},
  {4,1,0x2c,0xfa820u},{4,1,0x40,0xfaa30u},{4,2,0x2c,0x11e270u},{5,0,0x38,0xc0790u},{5,0,0x4c,0xbec50u},
  {5,0,0x80,0x2e4d60u},{5,1,0x10,0x31d400u},{5,1,0x14,0x31d400u},{5,1,0x18,0x31d400u},{5,1,0x1c,0x31d400u},
  {5,1,0x2c,0xfd200u},{5,1,0x40,0xfdde0u},{5,1,0x4c,0x100e70u},{5,1,0x58,0xff9f0u},{5,1,0x64,0x1023c0u},
  {6,0,0x38,0xc0790u},{6,0,0x4c,0xbec50u},{6,0,0x80,0x2e4d60u},{6,1,0x20,0x10f0e0u},{6,1,0x2c,0x10f260u},
  {6,1,0x30,0x10f1a0u},{6,1,0x3c,0x10f2f0u},{6,1,0x40,0x10f3a0u},{6,1,0x70,0x10f440u},{7,0,0x38,0xc0790u},
  {7,0,0x4c,0xbec50u},{7,0,0x80,0x2e4d60u},{7,1,0x10,0x10b510u},{7,1,0x14,0x10b540u},{7,1,0x18,0x10b590u},
  {7,1,0x1c,0x10b5b0u},{7,1,0x2c,0x10b5c0u},{7,1,0x30,0x10c790u},{7,1,0x34,0x10b730u},{7,1,0x3c,0x10b830u},
  {7,1,0x40,0x10b840u},{7,1,0x4c,0x10be20u},{7,1,0x6c,0x10c0b0u},{7,1,0x80,0x2e4d60u},{7,2,0x2c,0x117870u},
  {7,2,0x30,0x117700u},{7,2,0x3c,0x117900u},{7,2,0x40,0x1179c0u},{7,2,0x48,0x118120u},{7,2,0x4c,0x11e490u},
  {7,2,0x80,0x2e4d60u},{8,0,0x38,0xc0790u},{8,0,0x4c,0xbec50u},{8,0,0x80,0x2e4d60u},{8,1,0x10,0x10b510u},
  {8,1,0x14,0x10b540u},{8,1,0x18,0x10b590u},{8,1,0x1c,0x10b5b0u},{8,1,0x2c,0x10b5c0u},{8,1,0x30,0x10c790u},
  {8,1,0x34,0x10b730u},{8,1,0x3c,0x10b830u},{8,1,0x40,0x10b840u},{8,1,0x4c,0x10be20u},{8,1,0x6c,0x10c0b0u},
  {8,1,0x80,0x2e4d60u},{8,2,0x30,0x11e0b0u},{9,0,0x38,0xc0790u},{9,0,0x4c,0xbec50u},{9,0,0x80,0x2e4d60u},
  {9,1,0x10,0x10b510u},{9,1,0x14,0x10b540u},{9,1,0x18,0x10b590u},{9,1,0x1c,0x10b5b0u},{9,1,0x2c,0x10b5c0u},
  {9,1,0x30,0x10c790u},{9,1,0x34,0x10b730u},{9,1,0x3c,0x10b830u},{9,1,0x40,0x10b840u},{9,1,0x4c,0x10be20u},
  {9,1,0x6c,0x10c0b0u},{9,1,0x80,0x2e4d60u},{9,2,0x30,0x11e4f0u},{10,0,0x38,0xc0790u},{10,0,0x4c,0xbec50u},
  {10,0,0x80,0x2e4d60u},{10,1,0x2c,0x20fe50u},{10,1,0x30,0x20fe80u},{10,1,0x80,0x2e4d60u},
  {11,0,0x38,0xc0790u},{11,0,0x4c,0xbec50u},{11,0,0x80,0x2e4d60u},{11,1,0x2c,0x11e320u},{11,1,0x40,0x11e410u},
  {11,1,0x4c,0x11e490u},{12,0,0x38,0xc0790u},{12,0,0x4c,0xbec50u},{12,0,0x80,0x2e4d60u},{12,1,0x2c,0x11abf0u},
  {12,1,0x40,0x11ade0u},{12,1,0x44,0x11b260u},{12,1,0x80,0x2e4d60u},
};
// END 0x474B90
// The chain's callbacks in order; types outside the table refuse (the
// original would index past 0x474B90).
std::array<std::uint32_t,3> callbacks(std::uint32_t type,std::uint32_t slot,const char* owner) {
  if(type>=chain_levels.size())refuse(owner,"object type "+std::to_string(std::int32_t(type)));
  return object_type_callbacks(type,slot);
}
}

std::array<std::uint32_t,3> object_type_callbacks(std::uint32_t type,std::uint32_t slot) {
  std::array<std::uint32_t,3> out{};
  for(const auto& entry:entries)if(entry.type==type && entry.slot==slot)out[entry.level]=entry.value;
  return out;
}
std::array<std::uint32_t,3> object_type_chain(std::uint32_t type) {
  return type<chain_levels.size()?chain_levels[type]:std::array<std::uint32_t,3>{};
}

void placement_data_new(M& m,std::uint32_t request,std::uint32_t tag,std::uint32_t owner,std::uint32_t location) {
  zero_fill(m,request,0xc4u);                      // REP STOSD 0x31 dwords
  w32(m,request,tag);
  w32(m,request+0xcu,0);w32(m,request+0x18u,0);
  const auto forward=r32(m,0x474d08u);
  for(std::uint32_t n=0;n<3;++n)w32(m,request+0x28u+n*4u,r32(m,forward+n*4u));
  const auto up=r32(m,0x474d10u);
  for(std::uint32_t n=0;n<3;++n)w32(m,request+0x34u+n*4u,r32(m,up+n*4u));
  w32(m,request+0x58u,one);
  w32(m,request+0x74u,0);
  w16(m,request+0xb4u,0xffffu);
  w8(m,request+0x14u,0);
  NativeCampaignScriptBank bank(m);
  const auto body=native_campaign_script_tu15::object_try_and_get_and_verify_type(bank,owner,none);
  w32(m,request+0x64u,none);w32(m,request+0x5cu,none);
  if(body) {
    const auto flags=r32(m,request+0x18u);
    w32(m,request+0x60u,owner);
    w32(m,request+0x18u,(r8(m,body+0xc0u)>>2)&1u?flags|0x20u:flags&~0x20u);
    if((1u<<(r8(m,body+0xaau)&31u))&3u) {
      w32(m,request+0x5cu,r32(m,body+0x13cu));
      w32(m,request+0x64u,sx16(r16(m,body+0x138u)));
    }
  } else w32(m,request+0x60u,none);
  if(location) {
    for(std::uint32_t n=0;n<3;++n)w32(m,request+0x68u+n*4u,r32(m,location+n*4u));
  } else {w32(m,request+0x6cu,none);w32(m,request+0x68u,none);w16(m,request+0x70u,0xffffu);}
  w8(m,request+0xau,0xffu);w8(m,request+0xbu,0xffu);w16(m,request+8u,0xffffu);
  w32(m,request+4u,none);w32(m,request+0x10u,none);w8(m,request+0xb8u,0);
}

void object_world_position(M& m,std::uint32_t object,std::uint32_t out,std::uint32_t mxcsr) {
  namespace fp=native_campaign_script_fp;
  const auto rows=r32(m,r32(m,object_pool)+0x44u);
  const auto body=r32(m,rows+(object&0xffffu)*12u+8u);
  const auto parent=r32(m,body+0x14u);
  if(parent==none) {
    for(std::uint32_t n=0;n<3;++n)w32(m,out+n*4u,r32(m,body+0x64u+n*4u));
    return;
  }
  float x=f32(r32(m,body+0x64u)),y=f32(r32(m,body+0x68u)),z=f32(r32(m,body+0x6cu));
  const auto node=sx8(r8(m,body+0x18u))*0x34u;
  const auto parent_body=r32(m,rows+(parent&0xffffu)*12u+8u);
  const auto matrix=sx16(r16(m,parent_body+0x116u))+node+parent_body;
  const auto at=[&](std::uint32_t offset){return f32(r32(m,matrix+offset));};
  // UCOMISS/LAHF/TEST AH,44/JNP: skip scaling only when ordered-equal to 1.
  if(fp::sse_compared(r32(m,matrix),mxcsr)!=1.0f) {
    x=fp::mulss(at(0),x,mxcsr);y=fp::mulss(at(0),y,mxcsr);z=fp::mulss(at(0),z,mxcsr);
  }
  const auto row=[&](std::uint32_t forward,std::uint32_t left,std::uint32_t up,std::uint32_t origin) {
    auto sum=fp::addss(fp::mulss(at(up),z,mxcsr),fp::mulss(at(left),y,mxcsr),mxcsr);
    sum=fp::addss(sum,fp::mulss(at(forward),x,mxcsr),mxcsr);
    return fp::addss(sum,at(origin),mxcsr);
  };
  w32(m,out,bits(row(4,0x10,0x1c,0x28)));
  w32(m,out+4u,bits(row(8,0x14,0x20,0x2c)));
  w32(m,out+8u,bits(row(0xc,0x18,0x24,0x30)));
}

void object_refresh_spatial(M& m,std::uint32_t object) {
  const auto row=object_row(m,object);
  const auto body=r32(m,row+8u);
  if(!(r8(m,row+2u)&0x40u) || !((r32(m,body+4u)>>9)&1u))return;
  const auto packet=native_tu15_build_object_spatial_packet(m,object);
  native_tu15_update_object_spatial_chain(m,0x4eabb0u,r32(m,body+0x60u),object,packet);
}

void object_notify_nodes(M& m,std::uint32_t object,const ObjectNodesCallback& unit_nodes) {
  const auto body=object_body(m,object);
  const auto model_tag=r32(m,tag_data(m,r32(m,body))+0x38u);
  if(model_tag==none)return;
  const auto model=tag_data(m,model_tag);
  if(r32(m,model+4u)==none || r32(m,model+0x14u)==none)return;
  std::uint32_t count=0;
  const auto matrices=object_node_matrices(m,object,count);
  for(const auto callback:callbacks(sx8(r8(m,object_body(m,object)+0xaau)),type_slot_nodes,"C0200")) {
    if(!callback)continue;
    if(callback!=0x1157d0u || !unit_nodes)refuse("1157D0","unit node notification");
    unit_nodes(m,object,count,matrices);
  }
}

std::uint32_t physics_memory_pressure(const M& m,std::uint32_t mxcsr) {
  namespace fp=native_campaign_script_fp;
  // TU1.5 .rdata constants: 46A144, 456910, 46A140, 46A13C, 46A138, 46A134.
  constexpr std::uint32_t usage_scale=0x3c0ad8f3u,usage_limit=0x3f4ccccdu,level3=0x3f59999au,level2=0x3f400000u,
    level1=0x3f266666u,block_scale=0x358ad8f3u;
  const auto pool_level=[&](std::uint32_t global){
    const auto pool=r32(m,global);
    return pool && s32(r32(m,pool+0x20u))>0?3u:0u;
  };
  const auto first=pool_level(0x486074u),second=pool_level(0x486070u);
  std::uint32_t level=0;
  // COMISS limit,value; JBE keeps the current level (also when unordered).
  const auto below=[&](std::uint32_t limit,float value){
    return fp::sse_compared(limit,mxcsr)>fp::sse_compared(bits(value),mxcsr);
  };
  if(const auto owner=r32(m,0x48606cu)) {
    const auto usage=fp::mulss(fp::cvtsi2ss(s32(r32(m,owner+0xf0340u)),mxcsr),f32(usage_scale),mxcsr);
    level=3;
    if(below(usage_limit,usage)) {
      std::uint32_t sum=0;   // 2335B0: 16 weighted (current - previous) block counts
      for(std::uint32_t at=owner+0xf0384u,n=0;n<16;++n,at-=4u)sum+=(r32(m,at)-r32(m,at+0x44u))*r32(m,at-0xf02dcu);
      const auto blocks=fp::mulss(fp::cvtsi2ss(s32(sum),mxcsr),f32(block_scale),mxcsr);
      if(below(level3,blocks)) {
        level=2;
        if(below(level2,blocks)){level=1;if(below(level1,blocks))level=0;}
      }
    }
  }
  const auto largest=s32(level)<=s32(first)?first:level;
  if(s32(second)>s32(largest))return second;
  return s32(level)>s32(first)?level:first;
}

void physics_memory_reserve(M& m,std::uint32_t mxcsr) {
  if(r8(m,0x48b873u))return;
  const auto level=physics_memory_pressure(m,mxcsr);
  if(s32(level)>=2)refuse("1496F0","physics memory reclamation (pressure level "+std::to_string(level)+")");
  w8(m,0x48b873u,1);
  w8(m,0x48b873u,0);
}
}
