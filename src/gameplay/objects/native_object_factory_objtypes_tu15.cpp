// Native object type callbacks (track "objtypes"). Each owner replicates its
// TU1.5 routine over the live world memory: stores keep the original order,
// widths and values; x87 steps run the original instructions under the live
// control word. Oracle cases are in tools/native_object_factory_cases_objtypes.py
// and the harness kernels in tests/native_object_factory_objtypes_kernels.cpp.
#include "halo2/native_object_factory_objtypes_tu15.hpp"
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "halo2/native_object_factory_havok_tu15.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include "halo2/native_tu15_object_deletion.hpp"
#include "native_object_factory_tu15_internal.hpp"

namespace halo2::native_object_factory_tu15 {
using namespace detail;
namespace {
namespace fp=native_campaign_script_fp;
namespace animation=weapon_animation;  // the weapon track's native animation manager owners
constexpr std::uint32_t device_groups=0x4ecbf8u;      // device group data array
constexpr std::uint32_t scenario=0x4ecc20u;           // global scenario tag data
constexpr std::uint32_t structure_bsp=0x4ecc18u;      // current structure BSP
constexpr std::uint32_t structure_lightmap=0x4ecc14u; // current structure lightmap
constexpr std::uint32_t scenery_objects=0x52be70u;    // object per scenario scenery slot
constexpr std::uint32_t havok_world=0x52be74u;        // live Havok world (1CE340 gate)
constexpr std::uint32_t physics_components=0x52be5cu; // 0xA0-byte component data array
constexpr std::uint32_t network_owner=0x4dc04cu;      // B7E50 network state owner
constexpr std::uint32_t machine_flags=0x505e7cu;      // structure pathfinding bit vector
constexpr std::uint32_t machine_words=0x4f9b58u;      // +28: pathfinding words
constexpr std::uint32_t flag_46e468=0x46e468u;        // 2F30F0's byte
constexpr std::uint32_t random_seed=0x4f3cd8u;        // primary random seed pointer
constexpr std::uint32_t game_time=0x51df54u;          // +2 tick rate (int16), +8 ticks
constexpr std::uint32_t zero=0;                       // XORPS xmm0 bits

std::uint32_t object_type_mask(const M& m,std::uint32_t object) {
  return 1u<<(r8(m,object_row(m,object)+3u)&31u);  // x86 SHL masks the count
}
// The scenario scenery row named by body +1A: its +58 word less one, or -1
// for no row. 10F260 and 10F300 index [0x52BE70] with it.
std::uint32_t scenery_slot(const M& m,std::uint32_t body) {
  const auto index=r16(m,body+0x1au);
  if(index==0xffffu)return none;
  const auto rows=r32(m,r32(m,scenario)+0x54u);
  return sx16(r16(m,sx16(index)*0x5cu+rows+0x58u))-1u;
}
// 2F30F0: byte [0x46E468] = value.
void set_flag_46e468(M& m,std::uint8_t value){w8(m,flag_46e468,value);}
// 11E2C9..11E2FB: FILD draw (non-negative, so 11E2CF never adds 2^32),
// FMUL 1/65535, FADD 1.0, FMUL 10.0, FIMUL rate, FSTP dword, FLD, FISTP.
std::uint32_t garbage_delay(std::uint32_t draw,std::int32_t rate,std::uint16_t control) {
  const auto integer=std::int32_t(draw);
  const float unit=f32(0x37800080u),one_value=f32(one),ten=f32(0x41200000u);
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float stored{};std::int32_t result{};
  asm volatile("fnstcw %1\n\tfldcw %3\n\t"
      "fildl %4\n\tfmuls %5\n\tfadds %6\n\tfmuls %7\n\tfimull %8\n\tfstps %2\n\t"
      "flds %2\n\tfistpl %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved),"+m"(stored)
      :"m"(mode),"m"(integer),"m"(unit),"m"(one_value),"m"(ten),"m"(rate)
      :"st");
  return std::uint32_t(result);
#else
  (void)integer;(void)unit;(void)one_value;(void)ten;(void)rate;(void)control;
  refuse("11E2C9","x87 garbage delay on a non-x86 host");
#endif
}
constexpr std::uint32_t game_options=0x4f3218u;        // +8 campaign flag, +132 difficulty
constexpr std::uint32_t game_globals=0x4ecc1cu;        // +F8 flag, +FC difficulty table
constexpr std::uint32_t default_up=0x474d10u;          // pointer to the default up vector
constexpr std::uint32_t projectile_fuse=0x48a0b0u;     // int16 default (+132)
constexpr std::uint32_t random_unit=0x37800080u;       // [0x46A160] 1/65535
constexpr std::uint32_t spin_epsilon=0x38d1b717u;      // [0x46A0EC] 0.0001
constexpr std::uint32_t sourced_scale=0x3fc00000u;     // [0x449FD4] 1.5
constexpr std::uint32_t material_cont=0x636f6e74u;     // 'cont'
// FD3AE..FD40C: FLD upper, FLD lower, FILD draw, FMUL unit, FXCH st2,
// FSUB st0,st1, FMULP st2, FXCH st1, FADD st0,st1, FIMUL rate, FSTP dword,
// FSTP st0 (2AC760's interval scaled by the integer tick rate).
float random_interval_ticks(std::uint32_t draw,float lower,float upper,std::int32_t rate,std::uint16_t control) {
  const auto integer=std::int32_t(draw);const float unit=f32(random_unit);
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\t"
      "flds %3\n\tflds %4\n\tfildl %5\n\tfmuls %6\n\t"
      "fxch %%st(2)\n\tfsub %%st(1), %%st\n\tfmulp %%st, %%st(2)\n\t"
      "fxch %%st(1)\n\tfadd %%st(1), %%st\n\tfimull %7\n\tfstps %0\n\tfstp %%st(0)\n\t"
      "fldcw %1"
      :"=m"(result),"+m"(saved)
      :"m"(mode),"m"(upper),"m"(lower),"m"(integer),"m"(unit),"m"(rate)
      :"st","st(1)","st(2)");
  return result;
#else
  (void)integer;(void)unit;(void)lower;(void)upper;(void)rate;(void)control;
  refuse("FD3AE","x87 projectile interval on a non-x86 host");
#endif
}
// 100D6E..100DA5: FLD z, FMUL z, FLD y, FMUL y, FADDP, FLD x, FMUL x, FADDP,
// FSQRT, FST dword (the float length), FABS, FLD 0.0001, FCOMIP st1 (JA:
// epsilon above the extended magnitude), FSTP st0.
float spin_length(float x,float y,float z,std::uint16_t control,bool& still) {
  const float epsilon=f32(spin_epsilon);
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float length{};std::uint8_t above{};
  asm volatile("fnstcw %1\n\tfldcw %3\n\t"
      "flds %4\n\tfmuls %4\n\tflds %5\n\tfmuls %5\n\tfaddp\n\t"
      "flds %6\n\tfmuls %6\n\tfaddp\n\tfsqrt\n\tfsts %0\n\tfabs\n\t"
      "flds %7\n\tfcomip %%st(1), %%st\n\tfstp %%st(0)\n\tseta %2\n\t"
      "fldcw %1"
      :"=m"(length),"+m"(saved),"=m"(above)
      :"m"(mode),"m"(z),"m"(y),"m"(x),"m"(epsilon)
      :"st","st(1)","cc");
  still=above!=0;
  return length;
#else
  (void)x;(void)y;(void)z;(void)epsilon;(void)control;(void)still;
  refuse("100D6E","x87 projectile spin on a non-x86 host");
#endif
}
// 100DF7..100E32: FLD length, FMUL seconds, FLD st0, FSIN, FSTP dword sin,
// FCOS, FSTP dword cos.
void spin_rotation(float length,float seconds,std::uint16_t control,float& sine,float& cosine) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};
  asm volatile("fnstcw %2\n\tfldcw %3\n\t"
      "flds %4\n\tfmuls %5\n\tfld %%st(0)\n\tfsin\n\tfstps %0\n\tfcos\n\tfstps %1\n\t"
      "fldcw %2"
      :"=m"(sine),"=m"(cosine),"+m"(saved)
      :"m"(mode),"m"(length),"m"(seconds)
      :"st","st(1)");
#else
  (void)length;(void)seconds;(void)control;(void)sine;(void)cosine;
  refuse("100DF7","x87 projectile rotation on a non-x86 host");
#endif
}
// UCOMISS a,b + LAHF/TEST AH,44/JP: ordered equal under MXCSR.DAZ.
bool sse_equal(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr) {
  return fp::sse_compared(a,mxcsr)==fp::sse_compared(b,mxcsr);
}
// COMISS value,1.0 / JB: not below one (ordered) under MXCSR.DAZ.
bool sse_at_least_one(float value,std::uint32_t mxcsr) {return fp::sse_compared(bits(value),mxcsr)>=1.0f;}
constexpr std::uint32_t seconds_per_tick=0x3d088889u;  // [0x46A17C] 1/30
constexpr std::uint32_t frames_per_second=0x41f00000u; // [0x46A20C] 30.0
constexpr std::uint32_t scenery_animation=0x400000cu;  // 10F4A0's animation name
// 1E5360's search (EDX graph data); the packed animation, low word import
// (-1 for the graph itself), high word animation. 1E5C80 runs on a match.
std::uint32_t find_packed(M& m,std::uint32_t graph,std::uint32_t name) {
  auto packed=none,import=none,offset=0xffffffe0u;
  std::uint16_t found=0xffffu;
  if(!graph)return packed;
  auto current=graph;
  for(;;) {
    if(found!=0xffffu) {
      animation::graph_require_resident(m,graph,packed);  // 1E5C80
      return packed;
    }
    bool step=true;
    if(const auto count=s32(r32(m,current+0x2cu));count>0) {
      std::int32_t n=0;
      while(n<count && r32(m,r32(m,current+0x30u)+std::uint32_t(n)*0x6cu)!=name)++n;
      if(n<count) {
        found=std::uint16_t(n);
        packed=std::uint32_t(found)<<16|(import&0xffffu);
        step=found==0xffffu;
      } else found=std::uint16_t(packed>>16);
    }
    if(step) {
      ++import;offset+=0x20u;
      if(s32(import)>=s32(r32(m,graph+0x4cu)))return packed;
      const auto tag=r32(m,r32(m,graph+0x50u)+offset+4u);
      current=tag==none?0u:tag_data(m,tag);
      if(!current)return packed;  // 1E5428: the search found nothing
    }
  }
}
NativeObjectFactoryFp animation_fp(const ObjectTypeFp& floating_point) {
  return {floating_point.control_word,floating_point.mxcsr};
}
std::uint32_t next_random(M& m,std::uint32_t seed) {
  const auto value=r32(m,seed)*0x19660du+0x3c6ef35fu;
  w32(m,seed,value);
  return value;
}
}

// ---- owners the callbacks run ----
void network_entry_mark(M& m,std::uint32_t entry,std::uint32_t flags) {
  const auto owner=r32(m,network_owner);
  if(r32(m,owner+8u)!=4u)return;  // (mode 4 or 5) and not 3 and not 5
  const auto index=entry&0x3ffu;
  if(!(r8(m,r32(m,r32(m,owner+4u)+0x20a4u)+0x44u+index*8u)&4u))return;
  const auto at=r32(m,owner+4u)+index*0x24u+0x20b8u;
  w32(m,at,r32(m,at)|flags);
}

void physics_component_activate(M& m,std::uint32_t component,const ObjectTypeFp& fp) {
  // 1D9F10 with its 315CB0 body activation: the havok track's owner.
  auto state=fp;
  havok_component_activate(m,component,state);
}

void object_wake(M& m,std::uint32_t object,bool deferred,const ObjectTypeFp& fp) {
  const auto pool=r32(m,object_pool);
  const auto offset=(object&0xffffu)*12u;
  const auto rows=r32(m,pool+0x44u);
  const auto mask=1u<<(r8(m,rows+offset+3u)&31u);
  const auto body=r32(m,rows+offset+8u);
  const bool physics=(mask&0x1883u) && ((r8(m,body+0xc0u)>>6)&1u) && r32(m,body+0xb4u)!=none;
  bool mark=!deferred;
  if(!physics && deferred)w8(m,body+0xc1u,r8(m,body+0xc1u)|1u);
  if(mark) {
    if(physics) {
      const auto component=(r32(m,body+0xb4u)&0xffffu)*0xa0u+r32(m,r32(m,physics_components)+0x44u);
      physics_component_activate(m,component,fp);
    }
    w8(m,body+0xc1u,r8(m,body+0xc1u)&0xfeu);
    native_tu15_wake_object_ancestors(m,object);  // B9390
  }
  if(!(mask&0x1cu) && !(mask&0x20u))return;
  const auto entry=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u)+0xd4u;
  if(const auto value=r32(m,entry);value!=none)network_entry_mark(m,value,0x400u);
}

std::uint32_t lightmap_scenery_object_row(const M& m,std::uint32_t identifier,std::uint32_t checksum) {
  const auto lightmap=r32(m,structure_lightmap);
  if(!lightmap || s32(r32(m,lightmap+0x80u))<=0)return none;
  const auto group=r32(m,lightmap+0x84u);
  const auto count=s32(r32(m,group+0x58u));
  if(count<=0)return none;
  const auto type=r8(m,identifier+6u),source=r8(m,identifier+7u);
  const auto rows=r32(m,group+0x5cu),unique=r32(m,identifier);
  for(std::int32_t n=0;n<count;++n) {
    const auto row=rows+std::uint32_t(n)*12u;
    const auto row_source=r8(m,row+7u);
    bool match=(r8(m,row+6u)==type)&(row_source==source)&(r32(m,row)==unique);
    if(match && !row_source)match=r16(m,row+4u)==r16(m,identifier+4u);
    if(match && r32(m,row+8u)==checksum)return std::uint32_t(n);
  }
  return none;
}

void scenery_lightmap_object_index(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  const auto model=r32(m,tag_data(m,r32(m,body))+0x38u);
  auto index=none;
  if(model!=none && r16(m,tag_data(m,r32(m,body))+0xc0u)!=2u) {
    const auto render=r32(m,tag_data(m,model)+4u);
    if(render!=none)index=lightmap_scenery_object_row(m,body+0xa4u,r32(m,tag_data(m,render)+8u));
  }
  w32(m,body+0x134u,index);
}

void scenery_start_animation(M& m,std::uint32_t object,const ObjectTypeServices& services) {
  const auto body=object_body(m,object);
  w32(m,body+0x12cu,r32(m,body+0x12cu)&0xfffffffeu);
  if(r16(m,object_body(m,object)+0x12au)==0xffffu)return;
  const auto graph=r32(m,sx16(r16(m,body+0x12au))+body+0x68u);
  if(graph==none)return;
  if(object_set_animation(m,object,graph,scenery_animation,0,none,false,true,services))
    w32(m,body+0x12cu,r32(m,body+0x12cu)|1u);
}

std::uint32_t animation_graph_find_name(M& m,std::uint32_t graph,std::uint32_t name,std::uint32_t out) {
  const auto packed=find_packed(m,graph,name);
  w32(m,out,packed);
  return packed;
}

std::uint32_t animation_manager_find(M& m,std::uint32_t manager,std::uint32_t name,std::uint32_t out) {
  animation_graph_find_name(m,tag_data(m,r32(m,manager+0x68u)),name,out);
  return out;
}

void animation_manager_clear_overlays(M& m,std::uint32_t manager) {
  for(const std::uint32_t channel:{0x20u,0x40u}) {
    const auto at=[&](std::uint32_t offset){return manager+channel+offset;};
    w32(m,at(0),none);w16(m,at(4),0xffffu);w16(m,at(6),0xffffu);
    w32(m,at(0x1c),zero);w8(m,at(0x10),0);w8(m,at(0x11),0);w16(m,at(0x12),0);
    w32(m,at(0x18),one);w16(m,at(0x14),0);w16(m,at(0x16),0);
    w32(m,at(8),none);w8(m,at(0xc),0xffu);w8(m,at(0xd),0xffu);w16(m,at(0xe),0xffffu);
  }
}

bool animation_manager_start(M& m,std::uint32_t manager,std::uint32_t packed,std::uint32_t flags,
    const ObjectTypeFp& floating_point) {
  const auto controls=animation_fp(floating_point);
  // 1D5320(EAX 0xE0000C2, ECX manager; 0x7000101 x3, flags 4, start 0x3F);
  // its AL is not read.
  animation::manager_set_state(m,manager,0xe0000c2u,0x7000101u,0x7000101u,0x7000101u,4,0x3f,controls);
  if(r32(m,manager+0x68u)==none)return false;
  // 1D36C0(EAX manager, EBX flags; record manager, packed, key/bytes -1).
  const bool started=animation::manager_start_channel(m,manager,flags,manager,packed,none,0xffu,0xffu,0xffu,controls);
  if(started)animation_manager_clear_overlays(m,manager);  // 1D4F00
  return started;
}

void animation_manager_set_frame(M& m,std::uint32_t manager,std::uint32_t seconds,const ObjectTypeFp& floating_point) {
  if(r32(m,manager)==none || r16(m,manager+6u)==0xffffu)return;
  const auto frame=fp::mulss(f32(seconds),f32(frames_per_second),floating_point.mxcsr);
  animation::channel_set_frame(m,manager,bits(frame),animation_fp(floating_point));  // 1CE810
}

void object_physics_rebuild_check(M& m,std::uint32_t object,const ObjectTypeFp& fp) {
  const auto component=r32(m,object_body(m,object)+0xb4u);
  if(component==none)return;
  const auto flags=r32(m,r32(m,r32(m,physics_components)+0x44u)+(component&0xffffu)*0xa0u+4u);
  if((flags>>17)&1u)return;
  // 1CC431: 1CC310(object, 0), the havok track's owner (its 1D9510/1D73D0/
  // 1D97F0 are still refusals).
  auto state=fp;
  havok_component_state_flags(m,object,0,state);
}

void object_physics_deactivate(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  if(r32(m,body+0xb4u)!=none)refuse("1D9510","Havok component deactivation (physics work)");
  w8(m,body+0xc0u,r8(m,body+0xc0u)&0xbfu);
}

void object_header_activate(M& m,std::uint32_t object,const ObjectTypeFp& fp) {
  const auto row=r32(m,r32(m,object_pool)+0x44u)+(object&0xffffu)*12u;
  const auto flags=r8(m,row+2u);
  if(flags&1u)return;
  if(r16(m,row+4u)==0xffffu)return;
  const auto body=r32(m,row+8u);
  if(r32(m,body+0x14u)!=none)return;
  w8(m,row+2u,flags|1u);
  if((1u<<(r8(m,row+3u)&31u))&0x1883u)object_physics_rebuild_check(m,object,fp);
  if((r32(m,body+4u)>>14)&1u) {
    const auto collector=r32(m,0x4eabc4u);
    w16(m,collector+4u,std::uint16_t(r16(m,collector+4u)+1u));
  }
}

bool object_set_animation(M& m,std::uint32_t object,std::uint32_t graph_tag,std::uint32_t name,std::uint32_t start,
    std::uint32_t reference,bool flag_a,bool flag_b,const ObjectTypeServices& services) {
  const auto mxcsr=services.fp.mxcsr;
  if(object==none || graph_tag==none || !name)return false;
  const auto body=r32(m,r32(m,r32(m,object_pool)+0x44u)+(object&0xffffu)*12u+8u);
  if(r16(m,body+0x12au)==0xffffu)return false;
  const auto definition=tag_data(m,r32(m,body));
  const auto manager=sx16(r16(m,body+0x12au))+body;
  if(r32(m,manager+0x68u)!=graph_tag) {
    // 1D3380(ESI manager, EDI graph, EDX definition +38 model, 1) belongs to
    // the modelstate track.
    (void)definition;
    refuse("1D3380","animation manager setup for a new graph (modelstate)");
  }
  // 1D3FC0 stores the packed animation into 10F710's own argument slot.
  const auto packed=find_packed(m,tag_data(m,r32(m,manager+0x68u)),name);
  if((packed>>16)==0xffffu)return false;
  auto flags=flag_a?0x4001u:0x3fu;
  flags=flag_b?flags|2u:flags&0xfffffffdu;
  if(!animation_manager_start(m,manager,packed,flags,services.fp))return false;  // 1D4D90
  const auto seconds=fp::mulss(fp::cvtsi2ss(s32(sx16(std::uint16_t(start))),mxcsr),f32(seconds_per_tick),mxcsr);
  animation_manager_set_frame(m,manager,bits(seconds),services.fp);  // 1D4E30
  w32(m,body+0x12cu,r32(m,body+0x12cu)&0xfffffffeu);
  object_header_activate(m,object,services.fp);  // B92C0
  w32(m,body+0x138u,reference);
  // A reference runs BBA80, BA870, B95D0, B9390, BA630 and B92C0 again.
  if(reference!=none)refuse("BBA80","object reattachment for an animation reference (core)");
  object_wake(m,object,true,services.fp);
  if(!services.node_rebuild)refuse("BF050","node matrix rebuild after an animation start (core)");
  services.node_rebuild(m,object);
  return true;
}

// ---- "new" callbacks ----
bool device_new(M& m,std::uint32_t object,std::uint32_t,std::uint32_t) {
  const auto body=object_body(m,object);
  const auto at=[&](std::uint32_t offset){return body+offset;};
  w32(m,at(0x13c),none);w32(m,at(0x130),none);w32(m,at(0x18c),none);
  w16(m,at(0x190),0xffffu);w16(m,at(0x192),0xffffu);
  w32(m,at(0x1a8),zero);w32(m,at(0x1a4),one);w32(m,at(0x194),none);
  w16(m,at(0x19a),0xffffu);w8(m,at(0x198),0xffu);w8(m,at(0x199),0xffu);
  w8(m,at(0x19c),0);w8(m,at(0x19d),0);w16(m,at(0x19e),0);w16(m,at(0x1a0),0);w16(m,at(0x1a2),0);
  w32(m,at(0x1ac),none);w16(m,at(0x1b0),0xffffu);w16(m,at(0x1b2),0xffffu);
  w32(m,at(0x1c8),zero);w8(m,at(0x1bc),0);w8(m,at(0x1bd),0);w16(m,at(0x1be),0);
  w32(m,at(0x1c4),one);w16(m,at(0x1c0),0);w16(m,at(0x1c2),0);
  w32(m,at(0x1b4),none);w16(m,at(0x1ba),0xffffu);w8(m,at(0x1b8),0xffu);w8(m,at(0x1b9),0xffu);
  for(const auto offset:{0x14cu,0x150u,0x154u,0x158u,0x164u,0x15cu,0x160u,0x16cu,0x170u,0x174u,0x178u,0x184u,
                         0x17cu,0x180u})
    w32(m,at(offset),zero);
  return true;
}

bool machine_new(M& m,std::uint32_t object,std::uint32_t,std::uint32_t) {
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  w32(m,body+4u,r32(m,body+4u)|0x800u);
  auto flags=r32(m,body+4u);
  flags=(r8(m,definition+0x11eu)>>2)&1u?flags|0x1000u:flags&0xffffefffu;
  w32(m,body+4u,flags);
  flags=(r8(m,definition+0x11eu)>>2)&1u?flags|0x2000u:flags&0xffffdfffu;
  w32(m,body+4u,flags);
  w16(m,body+0x1dau,0xffffu);
  w16(m,body+0x1dcu,0);
  return true;
}

bool sound_scenery_new(M& m,std::uint32_t object,std::uint32_t,std::uint32_t) {
  const auto body=object_body(m,object);
  w32(m,body+4u,r32(m,body+4u)|0x10000u);
  return true;
}

bool crate_new(M& m,std::uint32_t object,std::uint32_t request,std::uint32_t) {
  const auto body=object_body(m,object);
  w32(m,body+0x12cu,0);
  if(const auto tag=r32(m,request);tag!=none) {
    const auto definition=tag_data(m,tag);
    if(r8(m,definition+0xbcu)&1u)w32(m,body+0x12cu,r32(m,body+0x12cu)|2u);
    const auto model=r32(m,definition+0x38u);
    const auto physics=model==none?none:r32(m,tag_data(m,model)+0x24u);
    if(physics!=none) {
      const auto data=tag_data(m,physics);
      bool moving=false;
      const auto count=s32(r32(m,data+0x38u));
      for(std::int32_t n=0;n<count;++n) {
        const auto motion=r16(m,r32(m,data+0x3cu)+std::uint32_t(n)*0x90u+0x1eu);
        if(motion!=1u && motion!=2u)moving=true;
      }
      if(!moving)w32(m,body+0x12cu,r32(m,body+0x12cu)|2u);
    }
  }
  // 212C70 fills a 0x54-byte query in 11E320's own frame (object, +B4, the
  // BC190 world transform, the model/physics tags and node matrix pointers);
  // 11E320 reads only its +48 physics model data. Those frame stores are
  // private to the original's stack, so only the tag walk is replicated.
  const auto model=r32(m,tag_data(m,r32(m,object_body(m,object)))+0x38u);
  if(model==none)return false;
  const auto physics=r32(m,tag_data(m,model)+0x24u);
  if(physics==none)return false;
  return s32(r32(m,tag_data(m,physics)+0x38u))>0;
}

bool scenery_new(M& m,std::uint32_t object,std::uint32_t,std::uint32_t,const ObjectTypeServices& services) {
  const auto body=object_body(m,object);
  const auto slot=scenery_slot(m,body);
  scenery_lightmap_object_index(m,object);
  w32(m,body+0x138u,none);
  w16(m,body+0x132u,3);
  object_wake(m,object,true,services.fp);
  scenery_start_animation(m,object,services);
  w32(m,body+0x12cu,0);
  if(slot!=none)w32(m,r32(m,scenery_objects)+slot*4u,object);
  return true;
}

bool garbage_new(M& m,std::uint32_t object,std::uint32_t,std::uint32_t,const ObjectTypeFp& floating_point) {
  const auto body=object_body(m,object);
  native_tu15_update_object_delete_schedule(m,object,true,0);  // BD980(object, 1, 0)
  w32(m,body+4u,r32(m,body+4u)|0x20000u);
  const auto seed=r32(m,random_seed);
  const auto draw=r32(m,seed)*0x19660du+0x3c6ef35fu;
  w32(m,seed,draw);
  const auto time=r32(m,game_time);
  const auto ticks=r32(m,time+8u);
  const auto delay=garbage_delay(draw>>16,s32(sx16(r16(m,time+2u))),floating_point.control_word);
  w32(m,body+0x16cu,ticks+delay);
  w32(m,body+4u,r32(m,body+4u)|0x10000u);
  return true;
}

std::uint32_t difficulty_scale(const M& m,std::uint16_t difficulty,std::uint16_t row) {
  const auto globals=r32(m,game_globals);
  if(!globals || !r32(m,globals+0xf8u))return one;
  const auto table=r32(m,globals+0xfcu);
  if(!table)return one;
  const auto level=std::int16_t(difficulty);
  const auto column=level<0?0:(level>3?3:level);
  return r32(m,table+std::uint32_t(column+s32(sx16(row))*4)*4u);
}

std::uint32_t projectile_damage_scale(const M& m,std::uint32_t tag,std::uint32_t source) {
  if(!((r32(m,tag_data(m,tag)+0xbcu)>>10)&1u))return one;
  if(source!=none)return sourced_scale;
  const auto options=r32(m,game_options);
  std::uint16_t difficulty=1;
  if(r32(m,options+8u)==1u)difficulty=r16(m,options+0x132u);
  return difficulty_scale(m,difficulty,10);
}

void projectile_spin(M& m,std::uint32_t object,const ObjectTypeFp& floating_point) {
  const auto mxcsr=floating_point.mxcsr;
  const auto body=object_body(m,object);
  const auto x=f32(r32(m,body+0x94u)),y=f32(r32(m,body+0x98u)),z=f32(r32(m,body+0x9cu));
  bool still=false;
  const auto length=spin_length(x,y,z,floating_point.control_word,still);
  if(!still) {
    const bool spinning=fp::sse_compared(bits(length),mxcsr)>fp::sse_compared(spin_epsilon,mxcsr);
    const auto inverse=fp::divss(f32(one),length,mxcsr);
    const auto axis_x=fp::mulss(inverse,x,mxcsr),axis_y=fp::mulss(y,inverse,mxcsr),axis_z=fp::mulss(z,inverse,mxcsr);
    if(spinning) {
      float sine{},cosine{};
      spin_rotation(length,f32(r32(m,r32(m,game_time)+4u)),floating_point.control_word,sine,cosine);
      w32(m,body+0x12cu,r32(m,body+0x12cu)|1u);
      w32(m,body+0x174u,bits(axis_x));w32(m,body+0x178u,bits(axis_y));w32(m,body+0x17cu,bits(axis_z));
      w32(m,body+0x180u,bits(sine));
      w32(m,body+0x184u,bits(cosine));
      return;
    }
  }
  w32(m,body+0x12cu,r32(m,body+0x12cu)&0xfffffffeu);
  w32(m,body+0x180u,zero);
  w32(m,body+0x184u,one);
}

bool projectile_new(M& m,std::uint32_t object,std::uint32_t request,std::uint32_t,const ObjectTypeFp& floating_point) {
  const auto mxcsr=floating_point.mxcsr;
  const auto offset=(object&0xffffu)*12u;
  const auto rows=r32(m,r32(m,object_pool)+0x44u);
  const auto body=r32(m,rows+offset+8u);
  const auto definition=tag_data(m,r32(m,body));
  const auto field=[&](std::uint32_t at){return r32(m,definition+at);};
  w32(m,body+0x12cu,r32(m,body+0x12cu)|2u);
  w32(m,body+0x144u,none);
  w16(m,body+0x148u,0xffffu);
  w16(m,body+0x130u,0);
  w16(m,body+0x132u,r16(m,projectile_fuse));
  const auto up=r32(m,default_up);
  for(std::uint32_t n=0;n<3;++n)w32(m,body+0x134u+n*4u,r32(m,up+n*4u));
  auto root=none;
  for(auto owner=r32(m,request+0x60u);owner!=none;owner=r32(m,r32(m,rows+(owner&0xffffu)*12u+8u)+0x14u))root=owner;
  w32(m,body+0x140u,root);
  w32(m,body+0x188u,one);
  w16(m,body+0x1a8u,0xffffu);
  w16(m,body+0x1aau,0);
  const auto scale=projectile_damage_scale(m,r32(m,body),r32(m,request+0x5cu));
  const auto seed=r32(m,random_seed);
  w32(m,body+0x16cu,scale);
  if(sse_equal(field(0x184),field(0x188),mxcsr))w32(m,body+0x18cu,field(0x184));
  else {
    const auto lower=f32(field(0x184)),upper=f32(field(0x188));
    const auto draw=next_random(m,seed)>>16;
    w32(m,body+0x18cu,bits(fp::random_interval(draw,lower,upper,f32(random_unit),floating_point.control_word)));
  }
  float rate_scaled{};
  std::uint32_t time{};
  if(((field(0xbc)>>2)&1u) || sse_equal(field(0xd4),field(0xd8),mxcsr)) {
    time=r32(m,game_time);
    rate_scaled=fp::mulss(fp::cvtsi2ss(s32(sx16(r16(m,time+2u))),mxcsr),f32(field(0xd4)),mxcsr);
  } else {
    const auto lower=f32(field(0xd4)),upper=f32(field(0xd8));
    const auto draw=next_random(m,seed)>>16;
    time=r32(m,game_time);
    rate_scaled=random_interval_ticks(draw,lower,upper,s32(sx16(r16(m,time+2u))),floating_point.control_word);
  }
  if(sse_at_least_one(rate_scaled,mxcsr))w32(m,body+0x15cu,bits(fp::divss(f32(one),rate_scaled,mxcsr)));
  const auto arming=fp::mulss(fp::cvtsi2ss(s32(sx16(r16(m,time+2u))),mxcsr),f32(field(0xcc)),mxcsr);
  if(sse_at_least_one(arming,mxcsr))w32(m,body+0x164u,bits(fp::divss(f32(one),arming,mxcsr)));
  w32(m,body+0x14cu,none);
  if(const auto count=s32(field(0x94));count>0) {
    const auto materials=field(0x98);
    for(std::int32_t n=0;;) {
      if(r32(m,materials+std::uint32_t(n)*24u)==material_cont){w32(m,body+0x14cu,std::uint32_t(n));break;}
      n=std::int16_t(n+1);
      if(n>=count)break;
    }
  }
  const auto speed=fp::mulss(f32(r32(m,body+0x16cu)),f32(field(0x17c)),mxcsr);
  for(std::uint32_t n=0;n<3;++n) {
    const auto product=fp::mulss(f32(r32(m,body+0x70u+n*4u)),speed,mxcsr);
    w32(m,body+0x88u+n*4u,bits(fp::addss(product,f32(r32(m,body+0x88u+n*4u)),mxcsr)));
  }
  w32(m,body+0x190u,bits(fp::mulss(f32(r32(m,body+0x16cu)),f32(field(0x17c)),mxcsr)));
  w32(m,body+0x12cu,r32(m,body+0x12cu)&0xfffffeffu);
  bool submerged=false;
  if(const auto cluster=r16(m,body+0x2cu);cluster!=0xffffu) {
    const auto bsp=r32(m,structure_bsp);
    const auto plane_index=r8(m,sx16(cluster)*0xb0u+r32(m,bsp+0xa0u)+0x70u);
    if(plane_index!=0xffu) {
      const auto plane=r32(m,bsp+0x68u)+(plane_index&0x7fu)*24u;
      if(r16(m,plane+2u)!=0xffffu) {
        if(!(plane_index&0x80u))submerged=true;
        else {
          const auto at=[&](std::uint32_t a){return f32(r32(m,a));};
          auto distance=fp::mulss(at(plane+0xcu),at(body+0x38u),mxcsr);
          distance=fp::addss(distance,fp::mulss(at(plane+8u),at(body+0x34u),mxcsr),mxcsr);
          distance=fp::addss(distance,fp::mulss(at(plane+4u),at(body+0x30u),mxcsr),mxcsr);
          distance=fp::subss(distance,at(plane+0x10u),mxcsr);
          submerged=fp::sse_compared(bits(distance),mxcsr)<0.0f;  // COMISS 0,d / JBE
        }
      }
    }
  }
  w32(m,body+4u,submerged?r32(m,body+4u)|8u:r32(m,body+4u)&0xfffffff7u);
  projectile_spin(m,object,floating_point);
  const auto current=object_body(m,object);
  const auto current_definition=tag_data(m,r32(m,current));
  w32(m,current+0x170u,(r32(m,current+4u)>>3)&1u?r32(m,current_definition+0x178u):r32(m,current_definition+0x16cu));
  const auto flags=r32(m,body+4u)|0x10000u;
  w32(m,body+4u,flags);
  w32(m,body+4u,flags|0x20000u);
  for(std::uint32_t n=0;n<3;++n)w32(m,body+0x198u+n*4u,r32(m,request+0x1cu+n*4u));
  w32(m,body+0x1a4u,none);
  auto owner=r32(m,request+0x60u);
  if(owner==none)return true;
  std::uint32_t row{};
  for(;;) {
    row=r32(m,r32(m,object_pool)+0x44u)+(owner&0xffffu)*12u;
    const auto parent=r32(m,r32(m,row+8u)+0x14u);
    if(parent==none)break;
    owner=parent;
  }
  if(!((1u<<(r8(m,row+3u)&31u))&1u))return true;  // biped root only
  const auto unit=r32(m,row+8u);
  const auto weapon=r32(m,unit+0x3e8u);
  if(weapon==none)return true;
  const auto unit_flags=r8(m,unit+0xc0u);
  if(!((unit_flags>>1)&1u) && !((unit_flags>>2)&1u) && weapon!=r32(m,unit+0x3e4u))return true;
  w32(m,body+0x12cu,r32(m,body+0x12cu)|0x8000u);
  w8(m,body+0xc0u,(r8(m,r32(m,row+8u)+0xc0u)>>2)&1u?r8(m,body+0xc0u)|4u:r8(m,body+0xc0u)&0xfbu);
  w8(m,body+0xc0u,(r8(m,r32(m,row+8u)+0xc0u)>>1)&1u?r8(m,body+0xc0u)|2u:r8(m,body+0xc0u)&0xfdu);
  w32(m,body+0xb8u,r32(m,r32(m,row+8u)+0xb8u));
  return true;
}

bool creature_new(M&,std::uint32_t,std::uint32_t,std::uint32_t) {
  refuse("11ABF0","creature new (1ED780 physics mode with Havok 1CC310, then 1D5320 and BC380 scale interpolation)");
}

std::map<std::uint32_t,ObjectTypeConstruct> objtypes_construct_callbacks(const ObjectTypeFp& floating_point) {
  ObjectTypeServices services;
  services.fp=floating_point;
  return objtypes_construct_callbacks(services);
}
std::map<std::uint32_t,ObjectTypeConstruct> objtypes_construct_callbacks(const ObjectTypeServices& services) {
  const auto floating_point=services.fp;
  return {
    {0x10b5c0u,device_new},{0x117870u,machine_new},{0x20fe50u,sound_scenery_new},{0x11e320u,crate_new},
    {0x10f260u,[services](M& m,std::uint32_t object,std::uint32_t request,std::uint32_t failed){
      return scenery_new(m,object,request,failed,services);}},
    {0x11abf0u,creature_new},
    {0x11e270u,[floating_point](M& m,std::uint32_t object,std::uint32_t request,std::uint32_t failed){
      return garbage_new(m,object,request,failed,floating_point);}},
    {0xfd200u,[floating_point](M& m,std::uint32_t object,std::uint32_t request,std::uint32_t failed){
      return projectile_new(m,object,request,failed,floating_point);}},
  };
}
std::map<std::uint32_t,ObjectTypeConstructFailed> objtypes_construct_failed_callbacks(const ObjectTypeServices& services) {
  return {{0x10b830u,device_new_failed},{0x117900u,machine_new_failed},
          {0x10f2f0u,[services](M& m,std::uint32_t object){scenery_new_failed(m,object,services);}}};
}
std::map<std::uint32_t,ObjectTypeConstructFailed> objtypes_construct_failed_callbacks() {
  return objtypes_construct_failed_callbacks(ObjectTypeServices{});
}

// ---- "new failed" callbacks ----
void device_new_failed(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  const auto groups=r32(m,device_groups);
  const auto release=[&](std::uint32_t group) {
    if(group!=none && (r8(m,r32(m,groups+0x44u)+(group&0xffffu)*12u+2u)&4u))
      native_tu15_data_array_delete(m,groups,group);  // 16E8A0
  };
  release(r32(m,body+0x130u));
  const auto second=r32(m,body+0x13cu);
  w32(m,body+0x130u,none);
  release(second);
  w32(m,body+0x13cu,none);
  for(const auto offset:{0x14cu,0x150u,0x154u,0x158u,0x164u,0x15cu,0x160u,0x16cu,0x170u,0x174u,0x178u,0x184u,
                         0x17cu,0x180u})
    w32(m,body+offset,zero);
}

void machine_new_failed(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  const auto count=r16(m,body+0x1dcu);
  if(std::int16_t(count)<=0)return;
  auto index=s32(sx16(r16(m,body+0x1dau)));
  if(index>=s32(sx16(count))+index)return;
  auto word=r32(m,machine_words)+std::uint32_t(index)*2u+0x28u;
  const auto bits=r32(m,machine_flags);
  do {
    const auto table=r32(m,r32(m,r32(m,structure_bsp)+0x220u)+0xcu);
    const auto bit=s32(sx16(r16(m,table+(std::uint32_t(index)&0xffffu)*2u)));
    const auto at=bits+std::uint32_t(bit>>5)*4u;
    w32(m,at,r32(m,at)|(1u<<(std::uint32_t(bit)&31u)));
    w16(m,word,0);  // FLD 0.0, FISTP: zero under every x87 control
    ++index;
    word+=2u;
  } while(index<s32(sx16(r16(m,body+0x1dcu)))+s32(sx16(r16(m,body+0x1dau))));
}

void scenery_new_failed(M& m,std::uint32_t object,const ObjectTypeServices& services) {
  const auto body=object_body(m,object);
  set_flag_46e468(m,0);
  if(r8(m,body+0x12cu)&2u)return;
  if(const auto slot=scenery_slot(m,object_body(m,object));slot!=none) {
    if(r32(m,havok_world)) {
      if(!services.phantom_wake)refuse("1CE340","Havok phantom wake around a released scenery (physics work)");
      services.phantom_wake(m,object);
    }
    w32(m,r32(m,scenery_objects)+slot*4u,none);
  }
  w32(m,body+0x12cu,r32(m,body+0x12cu)|2u);
}
}
