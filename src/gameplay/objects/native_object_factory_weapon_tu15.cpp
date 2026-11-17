// Native weapon construction owners (track "weapon"). Each owner replicates
// its TU1.5 routine; the oracle cases live in
// tools/native_object_factory_cases_weapon.py and the harness kernels in
// tests/native_object_factory_weapon_kernels.cpp. Floating-point steps run
// the original SSE/x87 instruction under the live control words.
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <bit>
#include <stdexcept>

namespace halo2::native_object_factory_tu15 {
namespace weapon_animation {
using namespace detail;
namespace {
namespace sse=native_campaign_script_fp;
constexpr std::uint32_t default_key=0x30000d9u;   // the mode tables' "any" key
constexpr std::uint32_t epsilon_bits=0x38d1b717u;  // [0x46A0EC] 1.0e-4
constexpr std::uint32_t blend_floor=0x3f7ff972u;   // [0x46A188] 0.9999
constexpr std::uint32_t blend_scale=0x3f80068eu;   // [0x46A184] 1.0002
constexpr std::uint32_t blend_select=0x3f0ccccdu;  // [0x46A234] 0.55
constexpr std::uint32_t ticks_per_second=0x41f00000u;  // [0x46A20C] 30.0
constexpr std::uint32_t random_unit=0x37800080u;   // [0x46A160] 1/65535
constexpr std::uint32_t streaming=0x51df20u;       // two enable bytes
constexpr std::uint32_t query_policy=0x47c1b4u;    // +0 selector, +1 blend query
constexpr std::uint32_t query_selectors=0x47c1b8u; // four {key, chained key}
constexpr std::uint32_t game_time=0x51df54u;
constexpr std::uint32_t random_seeds=0x4f3cd8u;

bool missing(std::uint32_t handle){return std::uint16_t(handle>>16)==0xffffu;}
// COMISS a,b (MXCSR.DAZ honoured): JA and JAE outcomes; unordered is false.
bool above(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return sse::sse_compared(a,mxcsr)>sse::sse_compared(b,mxcsr);}
bool at_least(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return sse::sse_compared(a,mxcsr)>=sse::sse_compared(b,mxcsr);}
bool equal(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return sse::sse_compared(a,mxcsr)==sse::sse_compared(b,mxcsr);}
std::uint32_t add(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(sse::addss(f32(a),f32(b),mxcsr));}
std::uint32_t sub(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(sse::subss(f32(a),f32(b),mxcsr));}
std::uint32_t mul(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(sse::mulss(f32(a),f32(b),mxcsr));}
std::uint32_t div(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(sse::divss(f32(a),f32(b),mxcsr));}
std::uint32_t convert(std::uint16_t value,std::uint32_t mxcsr){return bits(sse::cvtsi2ss(std::int16_t(value),mxcsr));}
// CVTSI2SS of an animation's signed frame count, minus [0x46A0EC].
std::uint32_t last_frame(const M& m,std::uint32_t animation,std::uint32_t mxcsr) {
  return sub(convert(r16(m,animation+0x14u),mxcsr),epsilon_bits,mxcsr);
}
// 1E58E0's FILD dword draw, FMUL dword [0x46A160], FSTP dword.
std::uint32_t unit_draw(std::uint32_t draw,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=std::uint16_t(control|0x3fu);std::uint16_t saved{};const auto integer=std::int32_t(draw);
  const auto unit=std::bit_cast<float>(random_unit);float result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(integer),"m"(unit):"st");
  return bits(result);
#else
  (void)draw;(void)control;
  throw std::runtime_error("Exact native TU1.5 animation variant draw backend unavailable");
#endif
}
// Animation row `index` (sign-extended word) of a graph; 0 for NONE.
std::uint32_t animation_row(const M& m,std::uint32_t graph,std::uint32_t index) {
  return index==none?0u:r32(m,graph+0x30u)+index*0x6cu;
}
// The graph that owns a handle's animation: a child for a non-NONE low word.
std::uint32_t handle_graph(const M& m,std::uint32_t graph,std::uint32_t handle) {
  return std::uint16_t(handle)==0xffffu?graph:graph_child(m,graph,sx16(std::uint16_t(handle)));
}
// 1E5AF0 (and its inlined copies): an animation flagged +16 bit 4 whose
// resource row (+24 into the graph's +B0 rows) is marked needs 126200.
void require_animation_resource(const M& m,std::uint32_t graph,std::uint32_t animation) {
  if(!(r8(m,animation+0x16u)&0x10u))return;
  if(r8(m,r32(m,graph+0xb0u)+r32(m,animation+0x24u)*20u+0x10u))
    refuse("126200","animation resource request");
}
// The +24/+28 (load) or +2C/+30 (unload) resource lists of a mode row.
void require_mode_resources(const M& m,std::uint32_t graph,std::uint32_t row,std::uint32_t list) {
  const auto count=s32(r32(m,row+list));
  for(std::int32_t n=0;n<count;++n)
    if(r8(m,r32(m,graph+0xb0u)+r32(m,r32(m,row+list+4u)+std::uint32_t(n)*4u)*20u+0x10u))
      refuse("126200","animation resource request");
}
}

// ---- Animation key names on MCC caches (see the header) ----
namespace {
thread_local bool mcc_query_names=false;
thread_local const QueryScope* innermost_query_scope=nullptr;
}
QueryNamespace::QueryNamespace(bool mcc):previous_(mcc_query_names){mcc_query_names=mcc;}
QueryNamespace::~QueryNamespace(){mcc_query_names=previous_;}
QueryScope::QueryScope(QueryOrigin origin,std::uint32_t state_key)
    :origin_(origin),state_key_(state_key),outer_(innermost_query_scope){innermost_query_scope=this;}
QueryScope::~QueryScope(){innermost_query_scope=outer_;}
std::uint32_t QueryScope::action_search_key(std::uint32_t key) {
  const QueryScope* scope=innermost_query_scope;
  // Only the state key 169863 entered 1D5320 with changes namespace; the
  // 0x400000C retry and 1D4F80's chained keys keep theirs.
  if(!mcc_query_names || !scope || scope->origin_!=QueryOrigin::first_person_state || key!=scope->state_key_)return key;
  for(const auto& name:first_person_state_names)
    if(name.tu15==key)return name.mcc;
  return key;
}
std::uint32_t QueryScope::class_search_key(std::uint32_t key) {
  const QueryScope* scope=innermost_query_scope;
  if(!mcc_query_names || !scope || scope->origin_==QueryOrigin::other || key!=dual_class_name.tu15)return key;
  return dual_class_name.mcc;
}

void FrameWord::bind(M& memory,std::uint32_t address){mirror_=&memory;address_=address;value_=r32(memory,address);}
void FrameWord::set(std::uint32_t value){value_=value;if(mirror_)w32(*mirror_,address_,value);}
void FrameWord::set_byte(std::uint8_t value){value_=(value_&~0xffu)|value;if(mirror_)w8(*mirror_,address_,value);}

std::uint32_t tag_block_search(const M& m,std::uint32_t block,std::uint32_t key,std::uint32_t size) {
  std::int32_t low=0,high=std::int32_t(r32(m,block)-1u);
  std::uint32_t found=0;
  for(bool last=false;!last && low<=high && !found;) {
    last=low==high;
    const auto middle=std::int32_t(std::uint32_t(high)+std::uint32_t(low))>>1;
    const auto row=std::uint32_t(middle)*size+r32(m,block+4u);
    const auto value=s32(r32(m,row));
    if(value==s32(key))found=row;
    else if(value>s32(key))high=middle-1;
    else low=middle+1;
  }
  return found;
}

std::uint32_t graph_child(const M& m,std::uint32_t graph,std::uint32_t index) {
  if(s32(index)>=s32(r32(m,graph+0x4cu)))return 0;
  const auto tag=r32(m,(index<<5)+r32(m,graph+0x50u)+4u);
  return tag==none?0u:tag_data(m,tag);
}

void graph_require_resident(const M& m,std::uint32_t graph,std::uint32_t handle) {
  if(!r8(m,streaming) || !r8(m,streaming+1u) || missing(handle))return;
  const auto owner=handle_graph(m,graph,handle);
  const auto index=sx16(std::uint16_t(handle>>16));
  auto row=animation_row(m,owner,index);
  if(const auto alias=r16(m,row+0x30u);alias!=0xffffu)row=animation_row(m,owner,sx16(alias));
  require_animation_resource(m,owner,animation_row(m,owner,index));
  while(r16(m,row+0x32u)!=0xffffu) {
    const auto variant=animation_row(m,owner,sx16(r16(m,row+0x32u)));
    require_animation_resource(m,owner,variant);
    row=variant;
  }
}

std::uint32_t graph_animation(const M& m,std::uint32_t graph,std::uint32_t handle) {
  if(missing(handle))return 0;
  const auto row=animation_row(m,handle_graph(m,graph,handle),sx16(std::uint16_t(handle>>16)));
  if(row)graph_require_resident(m,graph,handle);
  return row;
}

void manager_reset_channels(M& m,std::uint32_t manager) {
  for(std::uint32_t channel=0;channel<0x60u;channel+=0x20u) {
    const auto at=manager+channel;
    w32(m,at,none);w16(m,at+4u,0xffffu);w16(m,at+6u,0xffffu);w32(m,at+0x1cu,0);
    w8(m,at+0x10u,0);w8(m,at+0x11u,0);w16(m,at+0x12u,0);w32(m,at+0x18u,one);
    w16(m,at+0x14u,0);w16(m,at+0x16u,0);w32(m,at+8u,none);
    w8(m,at+0xcu,0xffu);w8(m,at+0xdu,0xffu);w16(m,at+0xeu,0xffffu);
  }
}

void resolve_keys(const M& m,std::uint32_t manager,std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d,
    FrameWord (&out)[4]) {
  out[0].set(a);out[1].set(b);out[2].set(c);out[3].set(d);
  bool current_missing=false;
  if(a==0x7000101u){const auto current=r32(m,manager+0x70u);out[0].set(current);current_missing=current==none;}
  if(current_missing || out[0]()==0x7000001u)out[0].set(0x6000086u);
  if(b==0x7000101u)out[1].set(r32(m,manager+0x74u));
  if(out[1]()==0x7000001u || out[1]()==none)out[1].set(0x7000083u);
  if(c==0x7000101u)out[2].set(r32(m,manager+0x78u));
  if(out[2]()==0x7000001u || out[2]()==none)out[2].set(default_key);
  auto second=out[1]();
  if(const auto row=tag_block_search(m,tag_data(m,r32(m,manager+0x68u))+0x54u,out[2](),8)) {
    const auto remap=r32(m,row+4u);
    if(remap!=none && remap!=default_key && second!=0x400054bu)second=remap;
  }
  out[1].set(second);
  if(d==0x7000101u){const auto current=r32(m,manager+0x7cu);out[3].set(current);if(current==none)out[3].set(0x400000cu);}
  if(out[3]()==0x7000001u)out[3].set(0x400000cu);
}

std::uint32_t mode_iterate(const M& m,ModeIterator& it,FrameWord& out0,FrameWord& out1,FrameWord& out2) {
  std::uint32_t found=0;
  do {
    if(it.state()>=8u)break;
    if(!it.row1()) {
      bool search=true;
      if(it.state()&4u){if(it.key0()==default_key)search=false;else out0.set(default_key);}
      else out0.set(it.key0());
      if(search){it.row1.set(tag_block_search(m,it.graph()+0x34u,out0(),0x14));it.row2.set(0);}
    }
    if(!it.row1()){it.state.set((it.state()+4u)&~3u);it.row2.set(0);continue;}
    if(!it.row2()) {
      bool search=true;
      if(it.state()&2u){if(it.key1()==default_key)search=false;else out1.set(default_key);}
      else out1.set(it.key1());
      if(search)it.row2.set(tag_block_search(m,it.row1()+4u,QueryScope::class_search_key(out1()),0x14));
      if(!it.row2()) {
        const auto state=(it.state()+2u)&~1u;it.state.set(state);
        if(!(state&2u))it.row1.set(0);
        continue;
      }
    }
    if(it.state()&1u){
      if(it.key2()!=default_key){out2.set(default_key);found=tag_block_search(m,it.row2()+4u,out2(),0x34);}
    } else {out2.set(it.key2());found=tag_block_search(m,it.row2()+4u,out2(),0x34);}
    const auto state=it.state()+1u;it.state.set(state);
    if(!(state&1u)){it.row2.set(0);if(!(state&2u))it.row1.set(0);}
  } while(!found);
  return found;
}

std::uint32_t mode_entry(const M& m,std::uint32_t graph,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2) {
  auto row=tag_block_search(m,graph+0x34u,k0,0x14);
  if(!row)return 0;
  row=tag_block_search(m,row+4u,k1,0x14);
  if(!row)return 0;
  return tag_block_search(m,row+4u,k2,0x34);
}

void mode_prefetch(const M& m,std::uint32_t graph,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2,bool load,bool unload) {
  if(!load && !unload)return;
  const auto row=mode_entry(m,graph,k0,k1,k2);
  if(!row)return;
  if(load)require_mode_resources(m,graph,row,0x24u);
  if(unload)require_mode_resources(m,graph,row,0x2cu);
}

void mode_prefetch_fallbacks(const M& m,std::uint32_t graph,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2,bool load,bool unload) {
  if(!load && !unload)return;
  mode_prefetch(m,graph,k0,k1,k2,load,unload);
  mode_prefetch(m,graph,k0,k1,default_key,load,unload);
  mode_prefetch(m,graph,k0,default_key,k2,load,unload);
  mode_prefetch(m,graph,k0,default_key,default_key,load,unload);
  mode_prefetch(m,graph,default_key,default_key,default_key,load,unload);
}

void graph_prefetch(const M& m,std::uint32_t graph,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2,bool load,bool unload) {
  if(!load && !unload)return;
  mode_prefetch_fallbacks(m,graph,k0,k1,k2,load,unload);
  for(std::int32_t n=0;n<s32(r32(m,graph+0x4cu));++n)
    mode_prefetch_fallbacks(m,graph_child(m,graph,std::uint32_t(n)),k0,k1,k2,load,unload);
}

void manager_prefetch(const M& m,std::uint32_t manager,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2,bool load,bool unload) {
  if(r32(m,manager+0x68u)==none)return;
  FrameWord keys[4];
  resolve_keys(m,manager,k0,k1,k2,0x7000101u,keys);
  graph_prefetch(m,tag_data(m,r32(m,manager+0x68u)),keys[0](),keys[1](),keys[2](),load,unload);
}

void animation_lookup(const M& m,std::uint32_t graph,std::uint32_t key3,std::uint32_t r0,std::uint32_t r1,std::uint32_t r2,
    FrameWord* out0,FrameWord* out1,FrameWord* out2,FrameWord& result) {
  if(r1==none){result.set(none);return;}
  ModeIterator it{FrameWord(graph),FrameWord(r0),FrameWord(r1),FrameWord(r2),FrameWord(0),FrameWord(0),FrameWord(0)};
  FrameWord k0(none),k1(none),k2(none);
  std::uint32_t found=none;
  while(const auto row=mode_iterate(m,it,k0,k1,k2)) {
    found=none;
    if(const auto entry=tag_block_search(m,row+4u,QueryScope::action_search_key(key3),8))found=r32(m,entry+4u);
    if(!missing(found))break;
  }
  if(out0)out0->set(k0());
  if(out1)out1->set(k1());
  if(out2)out2->set(k2());
  if(!missing(found)) {
    graph_require_resident(m,graph,found);
    graph_prefetch(m,graph,r0,r1,r2,true,false);
    graph_prefetch(m,graph,k0(),k1(),k2(),true,false);
  }
  result.set(found);
}

bool choose_animation(const M& m,std::uint32_t manager,FrameWord (&selected)[4],FrameWord (&resolved)[4],
    std::uint32_t a0,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t flags,FrameWord& result) {
  const auto graph=tag_data(m,r32(m,manager+0x68u));
  bool found=false;
  resolve_keys(m,manager,a0,a1,a2,a3,resolved);
  for(int n=0;n<4;++n)selected[n].set(resolved[n]());
  FrameWord handle;
  animation_lookup(m,graph,resolved[3](),resolved[0](),resolved[1](),resolved[2](),&selected[0],&selected[1],&selected[2],handle);
  result.set(handle());
  if(!missing(handle()))found=true;
  else {
    if(flags&2u) {
      animation_lookup(m,graph,0x400000cu,resolved[0](),resolved[1](),resolved[2](),&selected[0],&selected[1],&selected[2],handle);
      result.set(handle());
      if(!missing(handle())){selected[3].set(0x400000cu);found=true;}
    }
    if(missing(result()) && (flags&4u)) {
      const auto k0=resolved[0](),k1=resolved[1](),k2=resolved[2]();
      if(mode_entry(m,graph,k0,k1,k2))found=true;
      else if(mode_entry(m,graph,k0,k1,default_key)){found=true;selected[2].set(default_key);}
      else if(mode_entry(m,graph,k0,default_key,k2)){found=true;selected[1].set(default_key);}
      else if(mode_entry(m,graph,k0,default_key,default_key)) {
        found=true;selected[1].set(default_key);selected[2].set(default_key);
      } else if(mode_entry(m,graph,default_key,default_key,default_key)) {
        found=true;selected[0].set(default_key);selected[1].set(default_key);selected[2].set(default_key);
      }
    }
    if(!found)return false;
  }
  graph_prefetch(m,graph,selected[0](),selected[1](),selected[2](),true,false);
  return true;
}

void physical_query_find(const M& m,std::uint32_t row,FrameWord& out,std::uint32_t key,std::uint8_t b0,std::uint8_t b1,
    std::uint32_t k4,std::uint32_t k5,std::uint8_t c0,std::uint8_t c1,std::uint8_t c2) {
  const auto count=s32(r32(m,row+0x1cu));
  const auto rows=r32(m,row+0x20u);
  for(std::int32_t n=0;n<count;++n) {
    const auto entry=rows+std::uint32_t(n)*0x14u;
    if(r32(m,entry+4u)!=key || r8(m,entry+0xau)!=b0 || r8(m,entry+0xbu)!=b1)continue;
    const auto queries=s32(r32(m,entry+0xcu));
    auto query=r32(m,entry+0x10u);
    for(std::int32_t k=0;k<queries;++k,query+=0x14u)
      if(r32(m,query+4u)==k4 && r32(m,query+8u)==k5 && r8(m,query+0xeu)==c0 && r8(m,query+0xfu)==c1 && r8(m,query+0xcu)==c2) {
        out.set(r32(m,query+0x10u));return;
      }
  }
  out.set(none);
}

void physical_query_select(const M& m,std::uint32_t graph,std::uint32_t key1,FrameWord& out,std::uint32_t key0,
    std::uint32_t key2,std::uint32_t state,std::uint32_t a5,std::uint32_t a6,std::uint32_t a7,std::uint32_t a8,
    std::uint32_t a9,std::uint32_t a10,std::uint32_t a11) {
  if(key1==none){out.set(none);return;}
  ModeIterator it{FrameWord(graph),FrameWord(key0),FrameWord(key1),FrameWord(key2),FrameWord(0),FrameWord(0),FrameWord(0)};
  FrameWord k0(none),k1(none),k2(none);
  std::uint32_t last=none;
  while(const auto row=mode_iterate(m,it,k0,k1,k2)) {
    FrameWord query;
    physical_query_find(m,row,query,state,std::uint8_t(a5),std::uint8_t(a6),a7,a8,std::uint8_t(a9),std::uint8_t(a10),std::uint8_t(a11));
    if(!missing(query()))refuse("1E4C39","physical-query match selection");
    last=query();
  }
  out.set(last);
}

void physical_query_state(const M& m,std::uint32_t manager,FrameWord& out,std::uint32_t a1,std::uint32_t a2,
    FrameWord& flag,const Fp& fp) {
  flag.set_byte(0);
  if(!r8(m,query_policy+1u)){out.set(none);return;}
  const auto mxcsr=fp.mxcsr;
  auto blend=add(r32(m,manager+0x80u),epsilon_bits,mxcsr);
  if(above(0,blend,mxcsr))blend=0;else if(above(blend,blend_floor,mxcsr))blend=blend_floor;
  blend=mul(blend,blend_scale,mxcsr);
  if(above(0,blend,mxcsr))blend=0;else if(above(blend,one,mxcsr))blend=one;
  auto key=r32(m,manager+0x7cu);
  if(r8(m,query_policy) && r32(m,manager+0x20u)!=none && r16(m,manager+0x26u)!=0xffffu && above(blend,blend_select,mxcsr))
    for(std::uint32_t n=0;n<4u;++n)
      if(key==r32(m,query_selectors+8u*n)) {
        if(const auto chained=r32(m,query_selectors+8u*n+4u);chained!=none)key=chained;
        break;
      }
  const auto graph=tag_data(m,r32(m,manager+0x68u));
  FrameWord first;
  physical_query_select(m,graph,r32(m,manager+0x74u),first,r32(m,manager+0x70u),r32(m,manager+0x78u),key,none,none,a1,a2,none,none,0);
  const std::uint32_t chosen=none;  // 1D5050's +8 local; only 1D51FA (behind 1E4C39) sets it
  std::uint32_t animation=0;
  if(r16(m,manager+6u)!=0xffffu)animation=graph_animation(m,tag_data(m,r32(m,manager)),r32(m,manager+4u));
  if(animation) {
    // The CVTTSS2SI frame of +1C feeds only 1D51FA, reached through 1E4C39.
    for(std::uint32_t pass=1;pass<=4u;++pass) {
      FrameWord query;
      physical_query_select(m,tag_data(m,r32(m,manager+0x68u)),r32(m,manager+0x74u),query,r32(m,manager+0x70u),
          r32(m,manager+0x78u),r32(m,manager+0x7cu),none,none,a1,a2,none,none,pass);
    }
  }
  if(missing(first())) {
    if(missing(chosen)){flag.set_byte(0);out.set(none);}
    else out.set(chosen);
  } else out.set(missing(chosen)?first():chosen);
}

void physical_query_selector(const M& m,std::uint32_t key,FrameWord& result,std::uint32_t manager,std::uint32_t a1,
    std::uint32_t a2,std::uint32_t a3,FrameWord& mapped) {
  std::uint32_t chained=none;
  for(std::uint32_t n=0;n<4u;++n)
    if(key==r32(m,query_selectors+8u*n)){chained=r32(m,query_selectors+8u*n+4u);break;}
  mapped.set(none);
  if(!r8(m,query_policy) || chained==none){result.set(none);return;}
  FrameWord selected[4],resolved[4],handle(none);
  if(choose_animation(m,manager,selected,resolved,a1,a2,a3,chained,0,handle)) {
    mapped.set(chained);result.set(handle());
  } else result.set(none);
}

bool channel_stopped(const M& m,std::uint32_t record) {
  return !((r8(m,record+0x12u)&1u) && !(r8(m,record+0x11u)&9u));
}
std::uint32_t channel_animation(const M& m,std::uint32_t record) {
  if(r16(m,record+6u)==0xffffu)return 0;
  return graph_animation(m,tag_data(m,r32(m,record)),r32(m,record+4u));
}
std::uint32_t channel_valid(const M& m,std::uint32_t record) {
  return r32(m,record)!=none && r16(m,record+6u)!=0xffffu?1u:0u;
}
bool channel_finished(const M& m,std::uint32_t record) {
  if(r32(m,record)==none || r16(m,record+6u)==0xffffu)return true;
  return !(r8(m,graph_animation(m,tag_data(m,r32(m,record)),r32(m,record+4u))+0x18u)&1u);
}

bool channel_start(M& m,std::uint32_t record,std::uint32_t graph,std::uint32_t flags,std::uint32_t handle,
    std::uint32_t key,std::uint8_t b0,std::uint8_t b1,std::uint8_t b2) {
  if(graph==none || missing(handle))return false;
  w16(m,record+4u,0xffffu);w16(m,record+6u,0xffffu);
  w8(m,record+0x10u,0);w8(m,record+0x11u,0);w16(m,record+0x14u,0);w16(m,record+0x16u,0);
  w32(m,record+8u,key);w8(m,record+0xcu,b0);w16(m,record+0x12u,std::uint16_t(flags));w8(m,record+0xdu,b1);
  w32(m,record,graph);w32(m,record+0x1cu,0);w32(m,record+4u,handle);w32(m,record+0x18u,one);
  w16(m,record+0xeu,std::uint16_t(sx8(b2)));w8(m,record+0x10u,std::uint8_t((flags>>14)&2u));
  return true;
}

void channel_mark_events(M& m,std::uint32_t record,const Fp& fp) {
  const auto mxcsr=fp.mxcsr;
  w16(m,record+0x16u,0);
  if(r16(m,record+6u)==0xffffu)return;
  const auto animation=graph_animation(m,tag_data(m,r32(m,record)),r32(m,record+4u));
  std::uint32_t end=0;
  if(above(r32(m,record+0x18u),0,mxcsr))end=add(convert(r16(m,animation+0x14u),mxcsr),one,mxcsr);
  const auto frame=r32(m,record+0x1cu);
  const bool past=above(frame,end,mxcsr);
  const auto low=past?end:frame,high=past?frame:end;
  const auto events=s32(r32(m,animation+0x4cu));
  for(std::int32_t n=0;n<events;++n) {
    const auto event=r32(m,animation+0x50u)+std::uint32_t(n)*4u;
    const auto at=convert(r16(m,event+2u),mxcsr);
    const bool hit=equal(low,r32(m,record+0x1cu),mxcsr)
        ?above(at,low,mxcsr) && at_least(high,at,mxcsr)
        :at_least(at,low,mxcsr) && above(high,at,mxcsr);
    if(hit)w16(m,record+0x16u,std::uint16_t(r16(m,record+0x16u)|(1u<<(r8(m,event)&31u))));
  }
}

void channel_set_frame(M& m,std::uint32_t record,std::uint32_t frame,const Fp& fp) {
  const auto mxcsr=fp.mxcsr;
  std::uint32_t animation=0;
  if(r16(m,record+6u)!=0xffffu)animation=graph_animation(m,tag_data(m,r32(m,record)),r32(m,record+4u));
  std::uint32_t value=0;
  if(!above(0,frame,mxcsr)) {
    const auto last=last_frame(m,animation,mxcsr);
    value=above(frame,last,mxcsr)?last:frame;
  }
  w32(m,record+0x1cu,value);
  const auto flags=r8(m,record+0x11u);
  if((r8(m,record+0x12u)&1u) && !(flags&9u)) {
    auto ahead=mul(r32(m,r32(m,game_time)+4u),r32(m,record+0x18u),mxcsr);
    ahead=add(mul(ahead,ticks_per_second,mxcsr),value,mxcsr);
    const auto last=last_frame(m,animation,mxcsr);
    w8(m,record+0x11u,at_least(ahead,last,mxcsr)?std::uint8_t(flags|2u):std::uint8_t(flags&0xfdu));
  }
  channel_mark_events(m,record,fp);
}

void variant_alias(const M& m,std::uint32_t graph,FrameWord& out,std::uint32_t handle) {
  if(!missing(handle)) {
    const auto animation=graph_animation(m,graph,handle);
    if(r16(m,animation+0x30u)!=0xffffu || r16(m,animation+0x32u)!=0xffffu) {
      const auto alias=r16(m,graph_animation(m,graph,handle)+0x30u);
      if(alias!=0xffffu){out.set((handle&0xffffu)|std::uint32_t(alias)<<16);return;}
    }
  }
  out.set(handle);
}

void random_variant(M& m,std::uint32_t graph,FrameWord& out,std::uint32_t handle,bool primary,const Fp& fp) {
  if(missing(handle)){out.set(handle);return;}
  const auto mxcsr=fp.mxcsr;
  auto selected=handle;
  auto index=std::uint16_t(handle>>16);
  const auto animation=graph_animation(m,graph,handle);
  if(r16(m,animation+0x30u)!=0xffffu || r16(m,animation+0x32u)!=0xffffu) {
    const auto owner=handle_graph(m,graph,handle);
    FrameWord alias;
    variant_alias(m,graph,alias,handle);
    selected=alias();index=std::uint16_t(selected>>16);
    auto row=animation_row(m,owner,sx16(index));
    if(above(one,r32(m,row+0x1cu),mxcsr)) {
      const auto seeds=r32(m,random_seeds);
      const auto seed=primary?seeds:seeds+4u;
      const auto next=r32(m,seed)*0x19660du+0x3c6ef35fu;
      w32(m,seed,next);
      const auto draw=unit_draw(next>>16,fp.x87_control_word);
      if(r16(m,row+0x32u)!=0xffffu)
        do {
          if(at_least(r32(m,row+0x1cu),draw,mxcsr))break;
          index=r16(m,row+0x32u);
          selected=(selected&0xffffu)|std::uint32_t(index)<<16;
          row=animation_row(m,owner,sx16(index));
        } while(r16(m,row+0x32u)!=0xffffu);
    }
  }
  if(index!=0xffffu)graph_require_resident(m,graph,selected);
  out.set(selected);
}

void manager_variant(M& m,std::uint32_t manager,FrameWord& out,std::uint32_t handle,const Fp& fp) {
  if(missing(handle)){out.set(handle);return;}
  const auto graph=tag_data(m,r32(m,manager+0x68u));
  if(!graph){out.set(handle);return;}
  FrameWord variant;
  random_variant(m,graph,variant,handle,(r8(m,manager+0x6cu)>>1)&1u,fp);
  out.set(variant());
}

bool manager_start_channel(M& m,std::uint32_t manager,std::uint32_t flags,std::uint32_t record,std::uint32_t handle,
    std::uint32_t key,std::uint8_t b0,std::uint8_t b1,std::uint8_t b2,const Fp& fp) {
  if(r32(m,manager+0x68u)==none)return false;
  if(flags&0x10u){FrameWord variant;manager_variant(m,manager,variant,handle,fp);handle=variant();}
  if(!channel_start(m,record,r32(m,manager+0x68u),flags,handle,key,b0,b1,b2))return false;
  if(flags&4u)channel_set_frame(m,record,0,fp);
  if(flags&8u)w32(m,record+0x18u,one);
  return true;
}

bool manager_set_state(M& m,std::uint32_t manager,std::uint32_t key3,std::uint32_t a0,std::uint32_t a1,std::uint32_t a2,
    std::uint32_t flags,std::uint32_t start_flags,const Fp& fp,QueryOrigin origin) {
  const QueryScope names(origin,key3);
  const auto mxcsr=fp.mxcsr;
  bool result=false;
  FrameWord keys[4]{FrameWord(a0),FrameWord(a1),FrameWord(a2),FrameWord(key3)};
  FrameWord handle(none),selected[4];
  bool cycles=true;
  if(r32(m,manager)!=none && r16(m,manager+6u)!=0xffffu)
    cycles=!((r8(m,graph_animation(m,tag_data(m,r32(m,manager)),r32(m,manager+4u))+0x18u)>>1)&1u);
  if(flags&0x10u)manager_reset_channels(m,manager);
  w8(m,manager+0x6eu,r8(m,manager+0x6eu)&0xfeu);
  bool proceed=choose_animation(m,manager,selected,keys,a0,a1,a2,key3,flags,handle)
      && !missing(handle()) && !(flags&8u);
  std::uint32_t query=none;
  bool blend=false;
  if(proceed) {
    if(!(flags&0x200u)) {
      FrameWord out,flag;
      physical_query_state(m,manager,out,keys[0](),keys[3](),flag,fp);
      const auto unlocked=std::uint8_t(~r8(m,manager+0x6cu)&1u);
      query=out();
      const bool playing=r32(m,manager)!=none && r16(m,manager+6u)!=0xffffu && !channel_stopped(m,manager);
      blend=(flag()&unlocked&(flags>>10)&(playing?1u:0u)&1u)!=0;
      w8(m,manager+0x6eu,blend?std::uint8_t(r8(m,manager+0x6eu)|1u):std::uint8_t(r8(m,manager+0x6eu)&0xfeu));
    } else w8(m,manager+0x6eu,r8(m,manager+0x6eu)&0xfeu);
    if(missing(query) && blend)proceed=false;
  }
  if(proceed) {
    std::uint32_t phase=0;
    const bool timed=(flags&0x20u)!=0;
    if((timed || (flags&0x40u)) && r32(m,manager)!=none && r16(m,manager+6u)!=0xffffu) {
      const auto span=last_frame(m,channel_animation(m,manager),mxcsr);
      if(above(span,0,mxcsr)) {
        auto frame=r32(m,manager+0x1cu);
        if(above(0,frame,mxcsr))frame=0;else if(above(frame,span,mxcsr))frame=span;
        auto value=div(frame,span,mxcsr);phase=value;
        if(flags&0x40u){value=sub(one,value,mxcsr);phase=value;}
        if(above(0,value,mxcsr))phase=0;else if(above(value,one,mxcsr))phase=one;
      }
    }
    FrameWord chained(none),chained_key(none);
    if(!(flags&0x100u)) {
      FrameWord found;
      physical_query_selector(m,keys[3](),found,manager,keys[0](),keys[1](),keys[2](),chained_key);
      chained.set(found());
    }
    manager_reset_channels(m,manager);
    if(manager_start_channel(m,manager,start_flags,manager,handle(),selected[3](),0xffu,0xffu,0,fp)) {
      result=true;
      blend=channel_finished(m,manager) && cycles;
      // 1D55AA (a second channel for the blend query) needs a 1D5050 handle,
      // which only the 1E4C39 refusal could produce.
      if(!missing(query))refuse("1D55AA","physical-query blend channel");
      if(!channel_valid(m,manager+0x40u) && (timed || (flags&0x40u)) && above(phase,0,mxcsr))
        channel_set_frame(m,manager,mul(last_frame(m,channel_animation(m,manager),mxcsr),phase,mxcsr),fp);
      w8(m,manager+0x6cu,blend?std::uint8_t(r8(m,manager+0x6cu)&0xefu):std::uint8_t(r8(m,manager+0x6cu)|0x10u));
      if(!missing(chained()) && r32(m,manager+0x68u)!=none
          && channel_start(m,manager+0x20u,r32(m,manager+0x68u),0x3c0u,chained(),chained_key(),0xffu,0xffu,0))
        w32(m,manager+0x80u,0);
      w8(m,manager+0x6cu,r8(m,manager+0x6cu)&0xfeu);
    } else proceed=false;
  }
  if(!proceed) {
    if(!(flags&4u))return result;
    result=true;
  }
  const bool changed=r32(m,manager+0x70u)!=keys[0]() || r32(m,manager+0x74u)!=keys[1]() || r32(m,manager+0x78u)!=keys[2]();
  w32(m,manager+0x70u,keys[0]());w32(m,manager+0x74u,keys[1]());w32(m,manager+0x78u,keys[2]());w32(m,manager+0x7cu,keys[3]());
  if(!changed)return result;
  if(r32(m,manager+0x68u)!=none && !(r8(m,manager+0x6cu)&1u))
    manager_prefetch(m,manager,r32(m,manager+0x70u),r32(m,manager+0x74u),r32(m,manager+0x78u),true,true);
  const auto blend_channel=manager+0x40u;
  if(r32(m,blend_channel)!=none && r16(m,blend_channel+6u)!=0xffffu)return result;
  if(r32(m,blend_channel)!=none) {
    // +40 holds a graph but no animation here.
    if(r16(m,blend_channel+6u)!=0xffffu && (r8(m,blend_channel+0x12u)&1u))
      w8(m,blend_channel+0x11u,r8(m,blend_channel+0x11u)&0xfeu);
    if(r32(m,blend_channel)!=none && r16(m,blend_channel+6u)!=0xffffu)return result;
  }
  for(const auto channel:{manager,manager+0x20u})
    if(r32(m,channel)!=none && r16(m,channel+6u)!=0xffffu && (r8(m,channel+0x12u)&1u))
      w8(m,channel+0x11u,r8(m,channel+0x11u)&0xfeu);
  return result;
}
}

using namespace detail;

std::uint32_t weapon_owner(const SourcePhysicsContactMemory& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  return r8(m,body+0x12cu)&1u?r32(m,body+0x154u):none;
}

bool weapon_set_state(SourcePhysicsContactMemory& m,std::uint32_t object,std::uint32_t state,bool force,
    const NativeObjectFactoryFp& fp) {
  const auto body=object_body(m,object);
  {NativeCampaignScriptBank bank(m);native_campaign_script_tu15::object_mark_for_update(bank,object);}
  if(!force) {
    const auto current=s32(r32(m,body+0x178u));
    if(current && (current<=0 || current>2 || s32(state)<current))return false;
  }
  if(r16(m,object_body(m,object)+0x12au)!=0xffffu) {
    const auto manager=body+sx16(r16(m,body+0x12au));
    // 10AA06 jumps through an 11-entry table without a bounds check.
    static constexpr std::uint32_t keys[11]={0x400000cu,0x6000006u,0x6000007u,0x9000004u,0x9000005u,0x8000002u,
        0x8000002u,0x9000009u,0x9000009u,0x5000024u,0x8000025u};
    if(state>10u)refuse("10AA06","weapon state outside the authored animation table");
    const auto key=keys[state];
    if(weapon_owner(m,object)!=none)refuse("CFB40","owner animation variant (0x400054B)");
    if(weapon_animation::manager_set_state(m,manager,key,0x7000101u,0x7000001u,0x7000001u,
        key==0x400000cu?0x96u:0x82u,0x3fu,fp))
      w32(m,body+0x178u,state);
  }
  const auto tail=object_body(m,object);
  if((r8(m,tail+0x12cu)&1u) && r32(m,tail+0x154u)!=none)refuse("CC1E0","owner weapon state notification");
  return true;
}

bool weapon_new(SourcePhysicsContactMemory& m,std::uint32_t object,const NativeObjectFactoryFp& fp) {
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  w32(m,body+0x178u,0);w32(m,body+0x194u,none);w32(m,body+0x244u,none);w32(m,body+0x190u,0);w32(m,body+0x18cu,0);
  weapon_set_state(m,object,0,true,fp);
  w32(m,body+0x194u,none);w8(m,body+0x177u,0xffu);w32(m,body+0x24cu,0);
  w8(m,body+0x173u,0);w8(m,body+0x174u,0);w8(m,body+0x172u,0);w8(m,body+0x171u,0);
  // Each loop compares the sign-extended low word of a 32-bit counter.
  if(s32(r32(m,definition+0x2c0u))>0)
    for(std::uint32_t counter=0;;) {
      const auto index=sx16(std::uint16_t(counter));
      const auto magazine=index*0x5cu+r32(m,definition+0x2c4u);
      const auto slot=(index<<4)+body+0x224u;
      const auto total=r16(m,magazine+6u),rounds=r16(m,magazine+0xau);
      const auto loaded=std::int16_t(total)<=std::int16_t(rounds)?total:rounds;
      w16(m,slot+8u,loaded);
      const auto spare=s32(sx16(total))-s32(sx16(loaded)),limit=s32(sx16(r16(m,magazine+0xcu)));
      w16(m,slot+6u,std::uint16_t(spare<=limit?spare:limit));
      ++counter;
      if(s32(sx16(std::uint16_t(counter)))>=s32(r32(m,definition+0x2c0u)))break;
    }
  if(s32(r32(m,definition+0x2c8u))>0)
    for(std::uint32_t counter=0,index=0;;) {
      w32(m,body+index*12u+0x214u,none);
      index=sx16(std::uint16_t(++counter));
      if(s32(index)>=s32(r32(m,definition+0x2c8u)))break;
    }
  if(s32(r32(m,definition+0x2d0u))>0)
    for(std::uint32_t counter=0,index=0;;) {
      const auto trigger=index*0x34u+body+0x1a4u;
      w8(m,trigger,0x7f);w32(m,trigger+0x30u,none);
      index=sx16(std::uint16_t(++counter));
      if(s32(index)>=s32(r32(m,definition+0x2d0u)))break;
    }
  w16(m,body+0x17eu,0xffffu);
  const auto again=object_body(m,object);
  w16(m,again+0x254u,0xffffu);w16(m,again+0x256u,0xffffu);w32(m,again+0x250u,none);w32(m,again+0x258u,0);
  return true;
}

void item_restore_scale(SourcePhysicsContactMemory& m,std::uint32_t object,const NativeObjectFactoryFp& fp,
    const ItemScaleService& scale) {
  constexpr std::uint32_t half=0x3f000000u,three=0x40400000u;  // [0x46A0CC], [0x46A0E4]
  const auto body=object_body(m,object);
  std::uint32_t value=one;
  if(!(r8(m,body+0x12cu)&1u)) {
    const auto definition=tag_data(m,r32(m,body));
    const auto candidate=r32(m,definition+(r32(m,r32(m,0x4f3218u)+8u)==2u?0xc4u:0xc8u));
    if(native_campaign_script_fp::sse_compared(candidate,fp.mxcsr)>0.f) {
      if(native_campaign_script_fp::sse_compared(half,fp.mxcsr)>native_campaign_script_fp::sse_compared(candidate,fp.mxcsr))
        value=half;
      else if(native_campaign_script_fp::sse_compared(candidate,fp.mxcsr)>native_campaign_script_fp::sse_compared(three,fp.mxcsr))
        value=three;
      else value=candidate;
    }
  }
  if(!scale)refuse("B96B0","object scale service (BF050 node rebuild)");
  scale(object,value,0);
}

bool item_new(SourcePhysicsContactMemory& m,std::uint32_t object,const NativeObjectFactoryFp& fp,const ItemScaleService& scale) {
  const auto body=object_body(m,object);
  w32(m,body+4u,r32(m,body+4u)|0x1000u);
  w32(m,body+0x164u,0);w32(m,body+0x168u,one);w32(m,body+0x14cu,none);w32(m,body+0x154u,none);
  const auto vector=r32(m,0x474d10u);
  w32(m,body+0x158u,r32(m,vector));w32(m,body+0x15cu,r32(m,vector+4u));w32(m,body+0x160u,r32(m,vector+8u));
  w32(m,object_body(m,object)+0x150u,r32(m,r32(m,0x51df54u)+8u));
  item_restore_scale(m,object,fp,scale);
  return true;
}
}
