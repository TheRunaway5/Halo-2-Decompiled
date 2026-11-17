// Native attachment creators of TU1.5 object_new (track "attachments"): the
// looping sound (18BD80) and contrail (17E940 with 17EE10) creators that C0CD0
// calls, and the object function value subtree (BCB70, BC8F0, 10DE40 with the
// object callback BEC50, 10FB70 and 13E670 over the native 13DE70). Each owner
// replicates its original routine; the oracle cases are in
// tools/native_object_factory_cases_attachments.py and the harness kernels in
// tests/native_object_factory_attachments_kernels.cpp. Stack-only stores of
// the originals (locals and stack output slots) have no native counterpart.
#include "halo2/native_object_factory_attachments_tu15.hpp"
#include "halo2/native_object_factory_core_tu15.hpp"
#include "halo2/native_object_factory_nodebuild_tu15.hpp"
#include "halo2/native_object_factory_nodes_tu15.hpp"
#include "halo2/native_object_factory_placement_tu15.hpp"
#include "halo2/native_tu15_function_curves.hpp"
#include "halo2/native_tu15_object_deletion.hpp"
#include "halo2/native_tu15_object_detachment.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_function_evaluation_extended.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>

namespace halo2::native_object_factory_tu15 {
namespace {
using namespace detail;
namespace fp=native_campaign_script_fp;

// TU1.5 .rdata literals (checked against the XBE by the oracle cases).
constexpr std::uint32_t zero_literal=0x00000000u;      // 0x46A0E8
constexpr std::uint32_t unit_draw=0x37800080u;         // 0x46A160 (1/65535)
constexpr std::uint32_t upright_limit=0x3f7eb852u;     // 0x46A0F8 (0.995)
constexpr std::uint32_t looping_sounds=0x4f9b5cu;      // looping sound data array
constexpr std::uint32_t contrails=0x4f7214u;           // contrail data array
constexpr std::uint32_t function_variables=0x51daf4u;  // 10FB70 override rows
constexpr std::uint32_t game_time=0x51df54u;           // +4 tick seconds, +8 ticks
constexpr std::uint32_t random_seed=0x4f3cd8u;         // [[+0]+4] LCG state
constexpr std::uint32_t player_mode=0x4f3218u;         // BEC50 0xF0005AC: [+8] == 2
constexpr std::uint32_t object_globals=0x4eabc4u;      // BEC50 0x1A00071A..D: +70..+7C
constexpr std::uint32_t mode9_globals=0x52d710u;       // 24B430: +170 bytes, +1A0 words per player
constexpr std::uint32_t light_globals=0x51dae8u;       // C3910: +0 lights on, +4 the transition's start tick

void put32(M& m,OriginalOutput& out,std::uint32_t value){if(out.address)w32(m,out.address,value);out.value=value;}
void put8(M& m,OriginalOutput& out,std::uint8_t value){if(out.address)w8(m,out.address,value);out.value=value;}
float f(std::uint32_t value){return f32(value);}
// COMISS a, b under MXCSR.DAZ (JA: ordered and greater).
bool above(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){
  return fp::sse_compared(a,mxcsr)>fp::sse_compared(b,mxcsr);
}
// BEFF0 / 13E697 / BCB3E: zero when 0 > v, one when v > 1, else v (NaN kept).
std::uint32_t clamp_unit(std::uint32_t v,std::uint32_t mxcsr){
  if(above(zero_literal,v,mxcsr))return zero_literal;
  if(above(v,one,mxcsr))return one;
  return v;
}
// The LCG step the originals hash an object handle with, high word.
std::uint32_t handle_draw(std::uint32_t handle){return (handle*0x19660du+0x3c6ef35fu)>>16;}
// BEE63..BEE75: FILD dword draw (always non-negative here), FMUL dword
// [0x46A160], FSTP dword under the x87 control word.
std::uint32_t x87_draw(std::uint32_t draw,std::uint16_t control){
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};const auto integer=std::int32_t(draw);
  const auto unit=unit_draw;std::uint32_t result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(integer),"m"(unit):"st");
  return result;
#else
  (void)draw;(void)control;refuse("BEC50","x87 draw scaling off x86");
#endif
}
// BCA2B..BCA41: FILD dword draw, FMUL dword [0x46A160], FADD dword addend,
// FSTP dword.
std::uint32_t x87_draw_add(std::uint32_t draw,std::uint32_t addend,std::uint16_t control){
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};const auto integer=std::int32_t(draw);
  const auto unit=unit_draw;std::uint32_t result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tfadds %5\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(integer),"m"(unit),"m"(addend):"st");
  return result;
#else
  (void)draw;(void)addend;(void)control;refuse("BC8F0","x87 periodic phase off x86");
#endif
}
std::uint32_t mul(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(fp::mulss(f(a),f(b),mxcsr));}
std::uint32_t sub(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(fp::subss(f(a),f(b),mxcsr));}
std::uint32_t add(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(fp::addss(f(a),f(b),mxcsr));}
std::uint32_t div(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(fp::divss(f(a),f(b),mxcsr));}
std::uint32_t integer(std::uint32_t v,std::uint32_t mxcsr){return bits(fp::cvtsi2ss(s32(v),mxcsr));}
// 13E670/C2B30's range mapping of a 13DE70 result unless +1's high nibble is set.
std::uint32_t function_map(const M& m,std::uint32_t function,std::uint32_t v,std::uint32_t mxcsr){
  if(r8(m,function+1u)&0xf0u)return v;
  v=clamp_unit(v,mxcsr);
  const auto lower=r32(m,function+4u);
  return add(mul(sub(r32(m,function+8u),lower,mxcsr),v,mxcsr),lower,mxcsr);
}
}

std::uint32_t object_looping_sound_attachment_create(M& m,std::uint32_t tag,std::uint32_t object,std::uint32_t index,
    std::uint32_t primary_scale) {
  if(tag==none)return none;
  const auto pool=r32(m,looping_sounds);
  const auto handle=native_tu15_data_array_allocate(m,pool);
  if(handle==none)return none;
  const auto row=r32(m,pool+0x44u)+(handle&0xffffu)*0x18u;
  w32(m,row+0xcu,tag);
  w8(m,row+2u,3);
  w16(m,row+4u,0);
  w8(m,row+5u,std::uint8_t(r8(m,row+5u)|1u));
  if(object!=none) {
    w8(m,row+3u,1);
    w32(m,row+0x14u,primary_scale);
    w32(m,row+0x10u,object);
    w16(m,row+6u,std::uint16_t(index));
    return handle;
  }
  w16(m,row+6u,0xffffu);
  w8(m,row+3u,0);
  w32(m,row+0x10u,0);
  w8(m,row+0x14u,0xff);
  w16(m,row+0x16u,0xffffu);
  return handle;
}

bool object_function_variable(M& m,std::uint32_t object,std::uint32_t name,OriginalOutput& value,OriginalOutput& slot,
    std::uint32_t mxcsr) {
  put32(m,slot,none);
  if(object==none)return false;
  const auto rows=r32(m,function_variables);
  std::uint32_t n=0;
  for(;;++n) {
    if(n>=0x20u)return false;
    if(r32(m,rows+0x280u+n*4u)==object && r32(m,rows+n*0x14u)==name)break;
  }
  const auto row=rows+n*0x14u;
  const auto duration=r32(m,row+0x10u);
  const auto start=r32(m,row+4u),target=r32(m,row+8u);
  const auto elapsed=r32(m,r32(m,game_time)+8u)-r32(m,row+0xcu);
  if(s32(elapsed)<s32(duration) && duration) {
    auto t=div(integer(elapsed,mxcsr),integer(duration,mxcsr),mxcsr);
    t=mul(t,sub(target,start,mxcsr),mxcsr);
    put32(m,value,add(t,start,mxcsr));
  } else {
    put32(m,value,target);
  }
  put32(m,slot,n);
  return true;
}

// ---- BEC50's name callees ----
namespace {
constexpr std::uint32_t players=0x4f54f4u;              // player data array (0x21C rows)
constexpr std::uint32_t player_bits=0x51df9cu;          // [+0] word: 15B2D0's player mask
constexpr std::uint32_t scenario=0x4ecc20u;             // +10 word (BE9A0), +1C the north angle (C1F90)
constexpr std::uint32_t angle_two_pi=0x5564c4u;         // 2 pi and pi, set at startup (C1F90)
constexpr std::uint32_t angle_pi=0x5564c8u;
constexpr std::uint32_t inverse_two_pi=0x3e22f983u;     // 0x46A614 (1/(2 pi))
constexpr std::uint32_t minus_one=0xbf800000u;          // 0x46A0DC (-1.0)
constexpr std::uint32_t three=0x40400000u;              // 0x46A0E4 (3.0)
constexpr std::uint32_t quarter=0x3e800000u;            // 0x46A130 (0.25)
constexpr std::uint32_t three_quarters=0x3f400000u;     // 0x46A13C (0.75)
constexpr std::uint32_t fifteen=0x41700000u;            // 0x46A108 (15.0)
constexpr std::uint32_t twenty=0x41a00000u;             // 0x46A118 (20.0)
constexpr std::uint32_t fade_rate=0x3e4ccccdu;          // 0x46A16C (0.2)
constexpr std::uint32_t twelfth=0x3daaaaabu;            // 0x46A200 (1/12)
constexpr std::uint32_t sixtieth=0x3c888889u;           // 0x46A1FC (1/60)
constexpr std::uint32_t sixty_fourth=0x3c800000u;       // 0x46A384 (1/64)
constexpr std::uint32_t thirty_second=0x3d000000u;      // 0x46A388 (1/32)
constexpr std::uint32_t half_value=0x3f000000u;         // 0x46A0CC (0.5)

// BED09..BED1B: FLD y; FLD x; FPATAN; FSTP dword: atan2(y, x) under the
// x87 control word.
std::uint32_t x87_atan2(std::uint32_t y,std::uint32_t x,std::uint16_t control){
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tflds %4\n\tfpatan\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(y),"m"(x):"st","st(1)");
  return result;
#else
  (void)y;(void)x;(void)control;refuse("BEC50","x87 heading off x86");
#endif
}

// C1F90 (XMM0 angle, stack: reference): the angle less the reference,
// wrapped once: 2 pi off at pi or above, 2 pi on at -pi or below (pi and
// 2 pi are the globals the startup sets).
std::uint32_t wrapped_angle(const M& m,std::uint32_t angle,std::uint32_t reference,std::uint32_t mxcsr) {
  const auto pi=r32(m,angle_pi),two_pi=r32(m,angle_two_pi);
  auto x=sub(angle,reference,mxcsr);
  if(fp::sse_compared(x,mxcsr)>=fp::sse_compared(pi,mxcsr))x=sub(x,two_pi,mxcsr);
  if(fp::sse_compared(sub(zero_literal,pi,mxcsr),mxcsr)>=fp::sse_compared(x,mxcsr))x=add(two_pi,x,mxcsr);
  return x;
}

// BECEB (0x700055C): the object's heading as a fraction of a turn. Node 0
// nearly vertical (|forward z| >= 0.995, or NaN) is 1. Otherwise the
// heading of its forward axis (atan2(+4, +8)) relative to the scenario's
// north (+1C), wrapped, scaled by 1/(2 pi) and centred on 0.5.
std::uint32_t object_heading_value(M& m,std::uint32_t object,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto matrix=object_node_matrix(m,object,0);
  // FLD [+C], FABS, FLD 0.995, FCOMIP: unordered or |z| >= 0.995.
  if(!(f(upright_limit)>std::fabs(f(r32(m,matrix+0xcu)))))return one;
  const auto heading=x87_atan2(r32(m,matrix+4u),r32(m,matrix+8u),x87_control);
  const auto turn=wrapped_angle(m,heading,r32(m,r32(m,scenario)+0x1cu),mxcsr);
  return add(mul(turn,inverse_two_pi,mxcsr),half_value,mxcsr);
}

// BEAA0 (0x90006B5): [0x4EABC4] +14 is a tick (0: none). Without one, or
// before it, the value is 1. Otherwise the seconds since it (CVTSI2SS of
// the tick difference, MULSS by the tick length): 0 past one second, else
// (seconds - 15) * 0.2. (The original's 20-second test follows the
// one-second exit and never passes.)
std::uint32_t object_globals_timer_value(const M& m,std::uint32_t mxcsr) {
  const auto start=r32(m,r32(m,object_globals)+0x14u);
  if(!start)return one;
  const auto time=r32(m,game_time);
  const auto tick=r32(m,time+8u);
  if(s32(tick)<=s32(start))return one;
  const auto seconds=mul(integer(tick-start,mxcsr),r32(m,time+4u),mxcsr);
  if(above(seconds,one,mxcsr))return zero_literal;
  if(above(seconds,twenty,mxcsr))return one;
  return mul(sub(seconds,fifteen,mxcsr),fade_rate,mxcsr);
}

// 15B2D0 (ECX player, stack: 2 from BEC50): the player's scale. In game
// engine 7 ([0x4F3218] +180) for a player whose bit is set in [0x51DF9C]'s
// word: 3 with +22C bit 1, else by the team mode (+1C4): 0 for mode 1, 1
// otherwise. Without both, by the team mode: 0 for 1, 3 for 2, else 1.
// The player row's (0x21C bytes) +C1 index then scales it: 1 by 0.75, 2 by
// 0.5, 3 by 0.25; any other index keeps it.
std::uint32_t player_function_scale(const M& m,std::uint32_t player,std::uint32_t mxcsr) {
  const auto row=r32(m,r32(m,players)+0x44u)+(player&0xffffu)*0x21cu;
  const auto globals=r32(m,player_mode);
  const auto team_mode=r32(m,globals+0x1c4u);
  auto scale=one;
  if(r32(m,globals+0x180u)==7u && (r16(m,r32(m,player_bits))&(1u<<(player&31u)))) {
    if(r8(m,globals+0x22cu)&2u)scale=three;
    else if(team_mode==1u)scale=zero_literal;
  } else if(team_mode==1u)scale=zero_literal;
  else if(team_mode==2u)scale=three;
  switch(s32(sx8(r8(m,row+0xc1u)))) {
  case 1:return mul(scale,three_quarters,mxcsr);
  case 2:return mul(scale,half_value,mxcsr);
  case 3:return mul(scale,quarter,mxcsr);
  default:return scale;
  }
}

// BE9A0 (ECX object, EAX name, stack: value): the object's +24..+26 bytes.
// It answers (AL 1; BEC50 then sets `active` whatever the value) unless the
// object is NONE, or its +26 bit 3 is clear in a scenario whose +10 word is
// 0 (AL 0, value 0).
struct ObjectByteValue {std::uint8_t answered;std::uint32_t value;};
ObjectByteValue object_byte_value(const M& m,std::uint32_t object,std::uint32_t name,std::uint32_t mxcsr) {
  if(object==none)return {0,zero_literal};
  const auto body=object_body(m,object);
  const auto flags=r8(m,body+0x26u);
  const auto world=r32(m,scenario);
  if(!(flags&8u) && world && !r16(m,world+0x10u))return {0,zero_literal};
  switch(name) {
  case 0x180005fcu:return {1,(flags&1u)?zero_literal:one};
  case 0xd0005ffu:return {1,one};
  case 0x170005fau:return {1,mul(integer(r8(m,body+0x24u),mxcsr),sixty_fourth,mxcsr)};
  case 0x170005fbu:return {1,mul(integer(r8(m,body+0x25u),mxcsr),thirty_second,mxcsr)};
  case 0x1d0005fdu:return {1,(flags&2u)?one:minus_one};
  case 0x1d0005feu:return {1,(flags&4u)?one:minus_one};
  default:return {1,zero_literal};
  }
}

// The console clock. GetLocalTime (33BA6B: KeQuerySystemTime, the time
// zone bias, RtlTimeToTimeFields) unpacked by 13BB10 into dwords; the
// native reads the host's local time.
LocalTime console_local_time() {
  const auto now=std::time(nullptr);
  std::tm local{};
  localtime_r(&now,&local);
  return {std::uint32_t(local.tm_year+1900),std::uint32_t(local.tm_mon+1),std::uint32_t(local.tm_mday),
          std::uint32_t(local.tm_hour),std::uint32_t(local.tm_min),std::uint32_t(std::min(local.tm_sec,59))};
}
}

std::uint32_t local_holiday(const LocalTime& time) {
  switch(time.month) {
  case 12:return time.day==25u?1u:time.day==12u?6u:0u;
  case 1:return time.day==1u?2u:0u;
  case 10:return time.day==31u?3u:0u;
  case 7:return time.day==7u?4u:time.day==4u?5u:0u;
  default:return 0u;
  }
}

std::uint32_t holiday_function_value(std::uint32_t holiday) {
  switch(holiday) {
  case 1:return quarter;
  case 2:return half_value;
  case 3:return three_quarters;
  case 4:return one;
  default:return zero_literal;  // BEBE4: the local's 0.0
  }
}

std::uint32_t clock_hour_value(const LocalTime& time,std::uint32_t mxcsr) {
  auto hour=s32(time.hour);
  if(hour<1)hour=1;
  else if(hour>24)hour=24;
  return mul(integer(std::uint32_t(hour%12),mxcsr),twelfth,mxcsr);
}

std::uint32_t clock_minute_value(std::uint32_t minute,std::uint32_t mxcsr) {
  auto value=s32(minute);
  if(value<0)value=0;
  else if(value>59)value=59;
  return mul(integer(std::uint32_t(value),mxcsr),sixtieth,mxcsr);
}

namespace {
// C3910 (XMM0): the lights' global on/off transition. The seconds since the
// transition started ([0x51DAE8] +4 against the game tick, CVTSI2SS, MULSS
// by the tick length) at most 1 (COMISS; NaN takes 1), through transition
// curve 5 (17F8A0), mirrored (1 - value) while the lights are off (+0 byte).
std::uint32_t lights_transition_value(M& m,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto time=r32(m,game_time),lights=r32(m,light_globals);
  auto seconds=mul(integer(r32(m,time+8u)-r32(m,lights+4u),mxcsr),r32(m,time+4u),mxcsr);
  if(!above(one,seconds,mxcsr))seconds=one;
  NativeTu15PhysicsFrameFp frame{x87_control,0,mxcsr};
  const auto value=native_tu15_evaluate_transition_curve(m,5,seconds,frame,0);
  return r8(m,lights)?value:sub(one,value,mxcsr);
}
}

std::uint8_t object_base_function_value(M& m,std::uint32_t object,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,object);
  std::uint8_t handled=1;
  std::uint32_t v=zero_literal;
  bool clamp=false;
  bool always_active=false;  // BE9A0's answer (BL): `active` is 1 whatever the value
  switch(name) {
  case 0x500055bu:v=(r8(m,body+0x10au)>>2)&1u?zero_literal:one;break;
  case 0x30005a6u:case 0xa0006b9u:case 0x100006b4u:v=one;break;
  case 0x40005a7u:v=zero_literal;break;
  // (The upright heading's 1 skips the clamp in the original; it is 1 either way.)
  case 0x700055cu:v=object_heading_value(m,object,x87_control,mxcsr);clamp=true;break;
  case 0x90006b5u:v=object_globals_timer_value(m,mxcsr);clamp=true;break;
  // The console clock: BEBE0 (holiday), BEB00 (hour), BEB40 (minute), BEB90 (second).
  case 0x70006bau:v=holiday_function_value(local_holiday(console_local_time()));clamp=true;break;
  case 0xa0006b6u:v=clock_hour_value(console_local_time(),mxcsr);clamp=true;break;
  case 0xc0006b7u:v=clock_minute_value(console_local_time().minute,mxcsr);clamp=true;break;
  case 0xc0006b8u:v=clock_minute_value(console_local_time().second,mxcsr);clamp=true;break;
  case 0xd0005ffu:case 0x170005fau:case 0x170005fbu:case 0x180005fcu:case 0x1d0005fdu:case 0x1d0005feu: {
    const auto answer=object_byte_value(m,object,name,mxcsr);
    v=answer.value;always_active=answer.answered!=0;clamp=true;break;
  }
  case 0x700055du: {
    const auto model=tag_data(m,r32(m,tag_data(m,r32(m,body))+0x38u));
    const auto count=r32(m,model+0x50u);
    if(s32(count)<=0)break;
    auto variant=s32(sx8(r8(m,body+0xb1u)))+1;
    if(variant<0)variant=0;
    else if(variant>s32(count))variant=s32(count);
    v=div(integer(std::uint32_t(variant),mxcsr),integer(count,mxcsr),mxcsr);
    clamp=true;
    break;
  }
  case 0x130006bbu:v=lights_transition_value(m,x87_control,mxcsr);clamp=true;break;
  case 0xd000558u:v=r32(m,body+0xecu);clamp=true;break;
  case 0xf000559u:v=r32(m,body+0xf0u);clamp=true;break;
  case 0x15000557u:v=r32(m,body+0xf4u);clamp=true;break;
  case 0x13000556u:v=r32(m,body+0xf8u);clamp=true;break;
  case 0x1800055au:v=sub(r32(m,body+0xf0u),one,mxcsr);clamp=true;break;
  case 0x1a00071au:case 0x1a00071bu:case 0x1a00071cu:case 0x1a00071du:
    v=r32(m,r32(m,object_globals)+0x70u+(name-0x1a00071au)*4u);clamp=true;break;
  case 0xf0005aau:v=x87_draw(handle_draw(object),x87_control);clamp=true;break;
  case 0xf0005acu: {
    // A player's unit or biped in game mode 2 whose player scale (15B2D0)
    // is not above 0 is 0 (unclamped).
    if(r32(m,r32(m,player_mode)+8u)==2u && ((1u<<(r8(m,body+0xaau)&31u))&3u) && r32(m,body+0x13cu)!=none &&
       !above(player_function_scale(m,r32(m,body+0x13cu),mxcsr),zero_literal,mxcsr))
      break;
    // UCOMISS [+F0], 0; LAHF; TEST AH, 0x44; JP: only an ordered equal is 1.
    v=fp::sse_compared(r32(m,body+0xf0u),mxcsr)==0.0f?one:zero_literal;
    break;
  }
  default:handled=0;break;
  }
  if(clamp)v=clamp_unit(v,mxcsr);
  put32(m,value,v);
  put8(m,active,always_active || above(v,zero_literal,mxcsr)?1:0);
  return handled;
}

// ---- Type-level function value callbacks: C9070, E0900, 1046C0 ----
namespace {
constexpr std::uint32_t byte_unit=0x3b808081u;         // 0x46A0C4 (1/255)
constexpr std::uint32_t tenth=0x3dcccccdu;             // 0x46A178 (0.1)
// The callbacks' common tail: store the value, then active = value > 0;
// AL is 1 (C9160 / E09E1 / 104CF8).
std::uint8_t publish(M& m,OriginalOutput& value,OriginalOutput& active,std::uint32_t v,std::uint32_t mxcsr) {
  put32(m,value,v);
  put8(m,active,above(v,zero_literal,mxcsr)?1:0);
  return 1;
}
// E096F..E099F: FLD +90, +8C, +88; the original's x*x + y*y + z*z and
// FSQRT register sequence (24570's), FDIV dword, FSTP dword.
std::uint32_t x87_length_ratio(std::uint32_t x,std::uint32_t y,std::uint32_t z,std::uint32_t d,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[z]\n\tflds %[y]\n\tflds %[x]\n\t"
      ".byte 0xd9,0xc0,0xd8,0xc9,0xd9,0xc2,0xd8,0xcb,0xde,0xc1,0xd9,0xc3,0xd8,0xcc,0xde,0xc1,0xd9,0xfa,"
      "0xdd,0xdb,0xdd,0xd8,0xdd,0xd8\n\tfdivs %[d]\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[x]"m"(x),[y]"m"(y),[z]"m"(z),[d]"m"(d)
      :"st","st(1)","st(2)","st(3)","st(4)");
  return out;
#else
  (void)x;(void)y;(void)z;(void)d;(void)control;refuse("E0900","x87 speed ratio off x86");
#endif
}
// FC510 (ECX object): body +12C bit 0 without bit 1.
bool object_parent_mode(const M& m,std::uint32_t object) {
  const auto flags=r8(m,object_body(m,object)+0x12cu);
  return (flags&1u) && !(flags&2u);
}
// 165590 (ECX object): game mode +180 == 9 and the definition's +290 word 1.
bool mode9_object(const M& m,std::uint32_t object) {
  if(r32(m,r32(m,player_mode)+0x180u)!=9u || object==none)return false;
  return r16(m,tag_data(m,r32(m,object_body(m,object)))+0x290u)==1u;
}
}

std::uint32_t unit_function_ratio(const M& m,std::uint32_t unit,std::uint32_t mxcsr) {
  const auto body=object_body(m,unit);
  const auto current=r32(m,body+0x334u);
  if(!above(current,zero_literal,mxcsr))return zero_literal;
  const auto definition=tag_data(m,r32(m,body));
  const auto floor=r32(m,body+0x338u);
  // UCOMISS +338, 0; LAHF; TEST AH, 0x44; JP: only an ordered equal.
  if(fp::sse_compared(floor,mxcsr)==0.0f) {
    const auto maximum=r32(m,definition+0x1d4u);
    return above(maximum,current,mxcsr)?div(current,maximum,mxcsr):one;
  }
  if(!above(current,floor,mxcsr))return zero_literal;
  return div(sub(current,floor,mxcsr),r32(m,definition+0x1dcu),mxcsr);
}

std::uint32_t animation_channel_fraction(const M& m,std::uint32_t channel,std::uint32_t mxcsr) {
  if(r16(m,channel+6u)==0xffffu)return zero_literal;
  const auto animation=animation_definition(m,tag_data(m,r32(m,channel)),r32(m,channel+4u));
  if(!animation)refuse("1CF0E0","an animation channel whose 1E3150 lookup is empty (the original faults)");
  const auto frames=integer(sx16(r16(m,animation+0x14u)),mxcsr);
  if(!above(frames,zero_literal,mxcsr))return zero_literal;
  const auto v=div(r32(m,channel+0x1cu),frames,mxcsr);
  if(above(zero_literal,v,mxcsr))return zero_literal;
  return above(v,one,mxcsr)?one:v;
}

std::uint32_t unit_animation_fraction(const M& m,std::uint32_t unit,std::uint32_t mxcsr) {
  if(unit==none)return one;
  const auto body=object_body(m,unit);
  const auto manager=body+sx16(r16(m,body+0x12au));
  const auto state=r32(m,manager+0x7cu);
  if(state!=0x700005cu && state!=0x700005du)return one;
  const bool animated=r32(m,manager)!=none && r16(m,manager+6u)!=0xffffu;
  const auto fraction=animated?animation_channel_fraction(m,manager,mxcsr):zero_literal;
  return state==0x700005cu?fraction:sub(one,fraction,mxcsr);
}

std::uint32_t weapon_trigger_fraction(const M& m,std::uint32_t weapon,std::uint32_t index,std::uint32_t mxcsr) {
  const auto body=object_body(m,weapon);
  const auto definition=tag_data(m,r32(m,body));
  const auto n=sx16(std::uint16_t(index));
  if(s32(r32(m,definition+0x2c8u))<=s32(n))return zero_literal;
  const auto trigger=body+0x20cu+n*12u;
  const auto state=sx8(r8(m,trigger));
  if(state==2u)return one;
  if(state!=1u)return zero_literal;
  const auto ratio=div(integer(sx16(r16(m,trigger+2u)),mxcsr),r32(m,r32(m,definition+0x2ccu)+n*0x40u+0x1cu),mxcsr);
  return sub(one,mul(ratio,r32(m,r32(m,game_time)+4u),mxcsr),mxcsr);
}

// 24B430 (EAX weapon): the mode-9 value. 0 without a +17E player; 1 when
// the player's [0x52D710] +170 byte has bit 3; else game time +4 over
// [0x4F3218] +244 (1 for 0) times the player's positive +1A0 word.
std::uint32_t weapon_mode9_value(const M& m,std::uint32_t weapon,std::uint32_t mxcsr) {
  const auto body=object_body(m,weapon);
  const auto player=sx16(r16(m,body+0x17eu));
  const auto count=r16(m,r32(m,player_mode)+0x244u);
  const auto divisor=integer(count?sx16(count):1u,mxcsr);
  if(player==none)return zero_literal;
  const auto globals=r32(m,mode9_globals);
  if(r8(m,globals+player+0x170u)&8u)return one;
  const auto ticks=s32(sx16(r16(m,globals+player*2u+0x1a0u)));
  return mul(div(r32(m,r32(m,game_time)+4u),divisor,mxcsr),integer(ticks>0?std::uint32_t(ticks):0u,mxcsr),mxcsr);
}

std::uint8_t weapon_function_value(M& m,std::uint32_t weapon,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t,std::uint32_t mxcsr) {
  const auto body=object_body(m,weapon);
  const auto definition=tag_data(m,r32(m,body));
  std::uint32_t v=zero_literal;                          // XMM2
  bool clamp=true;                                       // 104CE6, else 104CF8
  // Not-taken max (COMISS v, x; JA): x replaces v unless v > x.
  const auto raise=[&](std::uint32_t x){if(!above(v,x,mxcsr))v=x;};
  // 1049EE / 104B13: network game mode 2, a +17E player and 165590, then
  // 1655F0, which only calls 24B430 (mode 9, implied by 165590).
  const auto mode9_value=[&](std::uint32_t& x)->bool {
    if(r32(m,r32(m,player_mode)+8u)!=2u || r16(m,body+0x17eu)==0xffffu || !mode9_object(m,weapon))return false;
    x=weapon_mode9_value(m,weapon,mxcsr);
    return true;
  };
  switch(name) {
  case 0x1d0007bau:
    if(r8(m,r32(m,object_globals)+0x82u)){clamp=false;break;}
    [[fallthrough]];
  case 0xc000569u: {                                     // 10476F: heat
    const auto barrels=s32(r32(m,definition+0x2d0u));
    for(std::int32_t i=0;i<barrels;++i)raise(r32(m,body+0x1bcu+std::uint32_t(i)*0x34u));
    const auto triggers=s32(r32(m,definition+0x2c8u));
    const auto rows=r32(m,definition+0x2ccu);
    for(std::int32_t i=0;i<triggers;++i) {
      const auto row=rows+std::uint32_t(i)*0x40u;
      if(above(r32(m,row+0x1cu),zero_literal,mxcsr))
        raise(mul(weapon_trigger_fraction(m,weapon,std::uint32_t(i),mxcsr),r32(m,row+0x28u),mxcsr));
      if(r8(m,body+0x20cu+std::uint32_t(i)*12u)==2u) {
        const auto rate=r32(m,row+0x28u);
        raise(add(mul(sub(one,rate,mxcsr),r32(m,body+0x188u),mxcsr),rate,mxcsr));
      }
    }
    raise(mul(r32(m,body+0x180u),r32(m,definition+0x164u),mxcsr));
    break;
  }
  case 0x1500056eu:case 0x1700056fu: {                   // 104CC5
    const std::uint32_t n=name!=0x1500056eu;
    if(s32(n)>=s32(r32(m,definition+0x2d0u))){clamp=false;break;}
    v=r32(m,body+0x1b8u+n*0x34u);
    break;
  }
  case 0x14000570u:case 0x16000571u: {                   // 104C6A
    const std::uint32_t n=name!=0x14000570u;
    if(s32(n)>=s32(r32(m,definition+0x2d0u))){clamp=false;break;}
    v=r32(m,body+0x1b4u+n*0x34u);
    break;
  }
  case 0x10000573u:case 0xe000572u: {                    // 104A98
    const std::uint32_t n=name!=0xe000572u;
    if(s32(n)>=s32(r32(m,definition+0x2d0u))){clamp=false;break;}
    const auto tick=r32(m,r32(m,game_time)+8u);
    v=r32(m,body+0x1b4u+n*0x34u);
    if(s32(tick-r32(m,body+0x248u))>1){v=zero_literal;clamp=false;}
    break;
  }
  case 0xf000574u:case 0x11000575u: {                    // 104B50
    const std::uint32_t n=name!=0xf000574u;
    if(s32(n)>=s32(r32(m,definition+0x2c8u))){clamp=false;break;}
    v=weapon_trigger_fraction(m,weapon,n,mxcsr);
    break;
  }
  case 0x1200056au:case 0x1400056du: {                   // 104B79
    const std::uint32_t n=name!=0x1200056au;
    if(s32(n)>=s32(r32(m,definition+0x2c0u))){clamp=false;break;}
    const auto capacity=r16(m,r32(m,definition+0x2c4u)+n*0x5cu+0xau);
    if(!capacity){clamp=false;break;}
    v=div(integer(sx16(r16(m,body+0x22cu+n*16u)),mxcsr),integer(sx16(capacity),mxcsr),mxcsr);
    break;
  }
  case 0x1700056bu:case 0x1700056cu: {                   // 104C1D / 104C8D: rounds digits
    if(s32(r32(m,definition+0x2c0u))<=0){clamp=false;break;}
    auto rounds=s32(sx16(r16(m,body+0x22cu)));
    if(name==0x1700056cu)rounds/=10;
    v=mul(integer(std::uint32_t(rounds%10),mxcsr),tenth,mxcsr);
    break;
  }
  case 0xb0005a9u:v=mul(integer(r8(m,body+0x175u),mxcsr),byte_unit,mxcsr);break;
  case 0x5000081u:v=r32(m,body+0x18cu);break;
  case 0x3000577u:v=r32(m,body+0x184u);break;
  case 0x4000567u:v=r32(m,body+0x180u);break;
  case 0x5000024u:clamp=false;if(object_parent_mode(m,weapon))v=one;break;
  case 0x9000568u:
    clamp=false;
    if(object_parent_mode(m,weapon) && !((r8(m,body+0x16cu)>>5)&1u))break;
    if(above(one,r32(m,body+0x184u),mxcsr))v=one;
    break;
  case 0xa000066u: {                                     // 10490F
    clamp=false;
    if(!((r8(m,body+0x16cu)>>1)&1u))break;
    const auto rate=r32(m,definition+0x150u);
    // UCOMISS rate, 1.0; LAHF; TEST AH, 0x44; JNP: an ordered equal is 0.
    if(fp::sse_compared(rate,mxcsr)==1.0f)break;
    v=div(sub(r32(m,body+0x180u),rate,mxcsr),sub(one,rate,mxcsr),mxcsr);
    clamp=true;
    break;
  }
  case 0x14000578u:clamp=false;if((r8(m,body+0x16eu)>>1)&1u)v=one;break;
  case 0x16000579u:clamp=false;if((r8(m,body+0x16eu)>>2)&1u)v=one;break;
  // 104A23..104A90 (unclamped): 0xF0006E5 x == 0, 0xE0006E6 0 < x < 1, 0xD0006E7 x == 1 (ordered).
  case 0xf0006e5u: {
    clamp=false;std::uint32_t x{};
    if(mode9_value(x) && fp::sse_compared(x,mxcsr)==0.0f)v=one;
    break;
  }
  case 0xe0006e6u: {
    clamp=false;std::uint32_t x{};
    if(mode9_value(x) && above(x,zero_literal,mxcsr) && above(one,x,mxcsr))v=one;
    break;
  }
  case 0xd0006e7u: {
    clamp=false;std::uint32_t x{};
    if(mode9_value(x) && fp::sse_compared(x,mxcsr)==1.0f)v=one;
    break;
  }
  case 0x120006e4u: {                                    // 104B48: x, clamped
    std::uint32_t x{};
    if(mode9_value(x))v=x;
    else clamp=false;
    break;
  }
  default:return 0;
  }
  if(clamp)v=clamp_unit(v,mxcsr);
  return publish(m,value,active,v,mxcsr);
}

std::uint8_t unit_function_value(M& m,std::uint32_t unit,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,unit);
  std::uint32_t v=zero_literal;
  bool clamp=true;                                       // C9148, else C9160
  switch(name) {
  case 0xf0006bfu:v=r32(m,body+0x1b8u);break;
  case 0x90006aau:v=unit_animation_fraction(m,unit,mxcsr);break;
  case 0xb0006abu:v=sub(one,unit_animation_fraction(m,unit,mxcsr),mxcsr);break;
  case 0x5000581u:v=unit_function_ratio(m,unit,mxcsr);break;
  case 0x6000087u:v=r32(m,body+0x2c4u);break;
  case 0xe00059fu:v=r32(m,body+0x208u);break;
  case 0x11000088u:v=r32(m,body+0x2b0u);break;
  case 0x1100059bu:v=r32(m,body+0x25cu);break;
  case 0x1100059cu:v=r32(m,body+0x260u);break;
  case 0x160005a0u:v=r32(m,body+0x264u);break;
  case 0xd00059du:v=mul(integer(r8(m,body+0x243u),mxcsr),byte_unit,mxcsr);break;
  case 0x1800059eu:v=mul(integer(r8(m,body+0x1f8u),mxcsr),byte_unit,mxcsr);break;
  case 0x90005a1u:
    clamp=false;
    if(!((r8(m,body+0x10au)>>2)&1u) && !((r32(m,body+0x134u)>>18)&1u))v=one;
    break;
  case 0x140005a2u:
    clamp=false;
    if(r32(m,body+0x248u)!=none || ((r32(m,body+0x134u)>>1)&1u))v=one;
    break;
  case 0x140005a3u:clamp=false;if(r32(m,body+0x24cu)!=none)v=one;break;
  case 0xb0005a9u:case 0x130005a8u: {                    // C927A: the current weapon's value
    const auto slot=sx8(r8(m,body+0x212u));
    if(slot==none)return 0;
    const auto weapon=r32(m,body+0x218u+slot*4u);
    if(weapon==none)return 0;
    return weapon_function_value(m,weapon,name==0xb0005a9u?0xb0005a9u:0xc000569u,value,active,x87_control,mxcsr);
  }
  default:return 0;
  }
  if(clamp)v=clamp_unit(v,mxcsr);
  return publish(m,value,active,v,mxcsr);
}

std::uint8_t biped_function_value(M& m,std::uint32_t biped,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,biped);
  const auto definition=tag_data(m,r32(m,body));
  std::uint32_t v=zero_literal;
  switch(name) {
  case 0x6000542u:if(r8(m,body+0x3dcu)==2u)v=one;break;
  case 0x70000c9u:
    if(!r8(m,body+0x39du))return 0;
    v=one;
    break;
  case 0xc0005a4u: {
    const auto scale=r32(m,definition+0x2dcu);
    if(!above(scale,zero_literal,mxcsr))return 0;
    v=x87_length_ratio(r32(m,body+0x88u),r32(m,body+0x8cu),r32(m,body+0x90u),scale,x87_control);
    break;
  }
  default:return 0;
  }
  // E09CD: the stored value is clamped; active tests the unclamped one,
  // which agrees with the clamped value's sign in every case.
  return publish(m,value,active,clamp_unit(v,mxcsr),mxcsr);
}

// ---- F9270: the vehicle level's callback ----
namespace {
constexpr std::uint32_t degree=0x3c8efa35u;            // 0x46A164 (pi/180)
constexpr std::uint32_t ratio_floor=0x3a83126fu;       // 0x46A180 (0.001)
constexpr std::uint32_t half_literal=0x3f000000u;      // 0x46A0CC (0.5)
constexpr std::uint32_t speed_offset=0x3e19999au;      // 0x44A8A0 / F9D0E push (0.15)
constexpr std::uint32_t fifth_literal=0x3e4ccccdu;     // 0x46A16C (0.2)
constexpr std::uint32_t contrail_speed=0x3e99999au;    // F9B5B push (0.3)
// FLD dword; FABS; FSTP dword: |x|, a signaling NaN quieted by the load.
std::uint32_t x87_abs(std::uint32_t x) {
  if((x&0x7f800000u)==0x7f800000u && (x&0x7fffffu))x|=0x400000u;
  return x&0x7fffffffu;
}
// FCOMI of two FABS results (exact extended loads): ordered a > b.
bool x87_above_abs(std::uint32_t a,std::uint32_t b) {
  if(a>0x7f800000u || b>0x7f800000u)return false;
  return a>b;
}
// The JA maxima of F9270 / F9E40 kind 2: |a| unless |b| >= |a| or unordered.
std::uint32_t x87_abs_max(std::uint32_t a,std::uint32_t b) {
  const auto x=x87_abs(a),y=x87_abs(b);
  return x87_above_abs(x,y)?x:y;
}
// F9418 / F9824: FLD dword; FMUL dword [0x46A164]; FABS; FSTP dword.
std::uint32_t x87_degrees_abs(std::uint32_t x,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t out{};const auto k=degree;
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[x]\n\tfmuls %[k]\n\tfabs\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[x]"m"(x),[k]"m"(k):"st");
  return out;
#else
  (void)x;(void)control;refuse("F9270","x87 angle scaling off x86");
#endif
}
// 24570 (EAX vector), FSTP dword: the x87 length (the original's register
// sequence, as E0900's).
std::uint32_t x87_length(std::uint32_t x,std::uint32_t y,std::uint32_t z,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[z]\n\tflds %[y]\n\tflds %[x]\n\t"
      ".byte 0xd9,0xc0,0xd8,0xc9,0xd9,0xc2,0xd8,0xcb,0xde,0xc1,0xd9,0xc3,0xd8,0xcc,0xde,0xc1,0xd9,0xfa,"
      "0xdd,0xdb,0xdd,0xd8,0xdd,0xd8\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[x]"m"(x),[y]"m"(y),[z]"m"(z)
      :"st","st(1)","st(2)","st(3)","st(4)");
  return out;
#else
  (void)x;(void)y;(void)z;(void)control;refuse("24570","x87 length off x86");
#endif
}
// F9B13..F9B3E: |+78 * +90 + +74 * +8C + +88 * +70| (x87), FSTP dword.
std::uint32_t x87_dot_abs(const std::array<std::uint32_t,6>& f,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[a]\n\tfmuls %[b]\n\tflds %[c]\n\tfmuls %[d]\n\t"
      "faddp %%st,%%st(1)\n\tflds %[e]\n\tfmuls %[g]\n\tfaddp %%st,%%st(1)\n\tfabs\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[a]"m"(f[0]),[b]"m"(f[1]),[c]"m"(f[2]),[d]"m"(f[3]),
       [e]"m"(f[4]),[g]"m"(f[5]):"st","st(1)");
  return out;
#else
  (void)f;(void)control;refuse("F9270","x87 dot product off x86");
#endif
}
// F98E0..F9943: |a| / |d| (x87 FDIV under the live precision), FSTP dword.
std::uint32_t x87_abs_quotient(std::uint32_t a,std::uint32_t d,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[d]\n\tfabs\n\tflds %[a]\n\tfabs\n\t"
      ".byte 0xd8,0xf1\n\tfstps %[o]\n\tfstp %%st(0)\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[a]"m"(a),[d]"m"(d):"st","st(1)");
  return out;
#else
  (void)a;(void)d;(void)control;refuse("F9270","x87 quotient off x86");
#endif
}
// F979D..F97DF: the three x87 cross terms (a*b - c*d, each FSUBP of two
// FMULs, FSTP dword), then F97E8..F9808: (c2 * +90 + c1 * +8C) + c0 * +88,
// FABS, FSTP dword (F9DA4). `f` is +70, +74, +78, +7C, +80, +84, +88, +8C, +90.
std::uint32_t x87_lateral_abs(const std::array<std::uint32_t,9>& f,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t out{};
  std::array<std::uint32_t,3> c{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds 8(%[f])\n\tfmuls 16(%[f])\n\tflds 4(%[f])\n\tfmuls 20(%[f])\n\tfsubrp %%st,%%st(1)\n\tfstps 0(%[c])\n\t"
      "flds 0(%[f])\n\tfmuls 20(%[f])\n\tflds 12(%[f])\n\tfmuls 8(%[f])\n\tfsubrp %%st,%%st(1)\n\tfstps 4(%[c])\n\t"
      "flds 12(%[f])\n\tfmuls 4(%[f])\n\tflds 0(%[f])\n\tfmuls 16(%[f])\n\tfsubrp %%st,%%st(1)\n\tfstps 8(%[c])\n\t"
      "flds 8(%[c])\n\tfmuls 32(%[f])\n\tflds 4(%[c])\n\tfmuls 28(%[f])\n\tfaddp %%st,%%st(1)\n\t"
      "flds 0(%[c])\n\tfmuls 24(%[f])\n\tfaddp %%st,%%st(1)\n\tfabs\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[f]"r"(f.data()),[c]"r"(c.data()),"m"(f),"m"(c)
      :"st","st(1)","memory");
  return out;
#else
  (void)f;(void)control;refuse("F9270","x87 lateral velocity off x86");
#endif
}
// F9C81 / F9CBA (FLD a; FSUB or FADD dword b), then FABS, FSTP dword.
std::uint32_t x87_abs_sum(std::uint32_t a,std::uint32_t b,bool subtract,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t out{};
  if(subtract)
    asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[a]\n\tfsubs %[b]\n\tfabs\n\tfstps %[o]\n\tfldcw %[saved]"
        :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[a]"m"(a),[b]"m"(b):"st");
  else
    asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[a]\n\tfadds %[b]\n\tfabs\n\tfstps %[o]\n\tfldcw %[saved]"
        :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[a]"m"(a),[b]"m"(b):"st");
  return out;
#else
  (void)a;(void)b;(void)subtract;(void)control;refuse("F9270","x87 tread velocity off x86");
#endif
}
// F9E10 (stack: a, b): a / b (DIVSS) when |b| > 0.001 (x87 FCOMI, exact), else 0.
std::uint32_t guarded_ratio(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr) {
  return x87_above_abs(x87_abs(b),ratio_floor)?div(a,b,mxcsr):zero_literal;
}
// F82D0 (EAX vehicle): body +384 set and either its age (+384 * tick
// seconds) below 0.5 or body +148 bit 1.
bool vehicle_recent(const M& m,std::uint32_t vehicle,std::uint32_t mxcsr) {
  const auto body=object_body(m,vehicle);
  const auto age=r8(m,body+0x384u);
  if(!age)return false;
  if(above(half_literal,mul(integer(age,mxcsr),r32(m,r32(m,game_time)+4u),mxcsr),mxcsr))return true;
  return r8(m,body+0x148u)&2u;
}
// E9530 (EAX object, ECX bit): bit `bit` of the +346 block's bitset at +4.
bool object_flag_bit(const M& m,std::uint32_t object,std::uint32_t bit) {
  const auto body=object_body(m,object);
  const auto block=body+sx16(r16(m,body+0x346u));
  return r32(m,block+std::uint32_t(s32(bit)>>5)*4u+4u)&(1u<<(bit&31u));
}
}

std::uint8_t vehicle_function_value(M& m,std::uint32_t vehicle,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,vehicle);
  const auto definition=tag_data(m,r32(m,body));        // EBP
  const auto field=[&](std::uint32_t at){return r32(m,body+at);};
  const auto def=[&](std::uint32_t at){return r32(m,definition+at);};
  std::uint32_t v=zero_literal;                          // XMM1
  bool clamp=true;                                       // F9DBA, else F9DD4
  const auto speed=[&]{return x87_length(field(0x88),field(0x8c),field(0x90),x87_control);};
  switch(name) {
  case 0x5000581u:
    if(field(0x148)&0x1000u){v=one;clamp=false;}
    else v=unit_function_ratio(m,vehicle,mxcsr);
    break;
  case 0x4000580u:
    clamp=false;
    if(vehicle_recent(m,vehicle,mxcsr) || (r8(m,body+0x148u)&2u) || ((r8(m,body+0x348u)>>5)&1u))v=one;
    break;
  case 0x800061eu:clamp=false;if(object_flag_bit(m,vehicle,0x32u))v=one;break;
  case 0x5000596u:v=field(0x370);break;
  case 0x6000595u:v=field(0x374);break;
  case 0xd00059au:v=field(0x3fc);break;
  case 0x900000eu:v=guarded_ratio(x87_abs(field(0x35c)),x87_degrees_abs(def(0x204),x87_control),mxcsr);break;
  case 0xa00000fu:v=guarded_ratio(x87_abs(field(0x35c)),x87_degrees_abs(def(0x208),x87_control),mxcsr);break;
  case 0xa00001au:v=guarded_ratio(x87_abs(field(0x358)),x87_abs(def(0x22c)),mxcsr);break;
  case 0xb00001bu:v=guarded_ratio(x87_abs(field(0x358)),x87_abs(def(0x230)),mxcsr);break;
  case 0xa000598u:v=guarded_ratio(field(0x378),def(0x264),mxcsr);break;
  case 0x13000588u:v=guarded_ratio(field(0x368),def(0x20c),mxcsr);break;
  case 0x14000589u:v=guarded_ratio(field(0x36c),def(0x20c),mxcsr);break;
  case 0x1700058eu:case 0x1800058cu:case 0x1800058fu:case 0x1900058du:
    v=guarded_ratio(field(0x360),def(0x20c),mxcsr);break;
  case 0xe00057au:v=guarded_ratio(x87_abs(field(0x354)),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);break;
  case 0xe00057du:v=guarded_ratio(x87_abs(field(0x358)),x87_abs_max(def(0x22c),def(0x230)),mxcsr);break;
  case 0xd00057fu: {
    const auto a=x87_degrees_abs(def(0x204),x87_control),b=x87_degrees_abs(def(0x208),x87_control);
    v=guarded_ratio(x87_abs(field(0x35c)),x87_above_abs(a,b)?a:b,mxcsr);
    break;
  }
  case 0xb000713u: {
    const auto seat=r8(m,body+0x38bu);
    if(seat==0xfeu || seat==0xffu){clamp=false;break;}
    const auto turn=guarded_ratio(x87_abs(field(0x358)),x87_abs_max(def(0x22c),def(0x230)),mxcsr);
    v=mul(add(turn,guarded_ratio(field(0x378),def(0x264),mxcsr),mxcsr),half_literal,mxcsr);
    break;
  }
  case 0x1300057eu: {
    const auto first=guarded_ratio(x87_abs(field(0x354)),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
    const auto second=guarded_ratio(x87_abs(field(0x358)),x87_abs_max(def(0x22c),def(0x230)),mxcsr);
    v=above(first,second,mxcsr)?first:second;
    break;
  }
  case 0xe000583u:
    if(!(r8(m,body+4u)&0xcu)){clamp=false;break;}
    v=guarded_ratio(speed(),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
    break;
  case 0xf000584u:
    if(!r8(m,body+0x34fu)){clamp=false;break;}
    [[fallthrough]];
  case 0xc000582u:v=guarded_ratio(speed(),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);break;
  case 0x10000585u: {
    const auto dot=x87_dot_abs({field(0x78),field(0x90),field(0x74),field(0x8c),field(0x88),field(0x70)},x87_control);
    v=guarded_ratio(dot,x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
    break;
  }
  case 0xe00057cu: {
    const auto throttle=field(0x354);
    const auto low=above(throttle,zero_literal,mxcsr)?zero_literal:throttle;
    const auto scale=def(0x1f8);
    v=x87_above_abs(x87_abs(scale),ratio_floor)?x87_abs_quotient(low,scale,x87_control):zero_literal;
    break;
  }
  case 0xd00057bu: {
    const auto throttle=field(0x354);
    v=guarded_ratio(above(zero_literal,throttle,mxcsr)?zero_literal:throttle,x87_abs(def(0x1f4)),mxcsr);
    break;
  }
  case 0x14000599u: {
    const auto ratio=guarded_ratio(speed(),x87_abs(def(0x1f4)),mxcsr);
    v=guarded_ratio(sub(mul(ratio,field(0x374),mxcsr),speed_offset,mxcsr),speed_offset,mxcsr);
    break;
  }
  // F9E40 kind 2 (the larger of |+1F4| and |+1F8|, ties and NaN to +1F8)
  // over an x87 velocity term (F9DA4: FABS, FSTP dword).
  case 0xb000587u:                                       // F9484: velocity_up
    v=guarded_ratio(x87_dot_abs({field(0x90),field(0x84),field(0x8c),field(0x80),field(0x7c),field(0x88)},x87_control),
        x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
    break;
  case 0xd000586u:                                       // F979D: the lateral velocity
    v=guarded_ratio(x87_lateral_abs({field(0x70),field(0x74),field(0x78),field(0x7c),field(0x80),field(0x84),
        field(0x88),field(0x8c),field(0x90)},x87_control),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
    break;
  case 0x1300058au:                                      // F9C78: left_tread_velocity
    v=guarded_ratio(x87_abs_sum(field(0x354),field(0x35c),true,x87_control),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
    break;
  case 0x1400058bu:                                      // F9CB1: right_tread_velocity
    v=guarded_ratio(x87_abs_sum(field(0x35c),field(0x354),false,x87_control),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
    break;
  case 0x17000592u:case 0x18000590u:case 0x18000593u:case 0x19000591u:  // F9D95: the tire velocities
    v=guarded_ratio(x87_abs(field(0x354)),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
    break;
  case 0xb000597u:case 0xc0005a5u: {                     // F95FB: engine_hack, engine_power
    const auto mode=r16(m,definition+0x1f0u);
    if(mode==1u || mode==0u)v=guarded_ratio(field(0x378),def(0x264),mxcsr);   // F970A
    else {
      // FA5D0 (ECX 7, stack +348): bit 7 of the word.
      const bool boosted=(r16(m,body+0x348u)>>7)&1u;
      const auto velocity=body+(boosted?0x3f0u:0x88u);
      const auto throttle=field(boosted?0x3dcu:0x354u);
      const auto r1=guarded_ratio(x87_dot_abs({field(0x78),r32(m,velocity+8u),field(0x74),r32(m,velocity+4u),
          field(0x70),r32(m,velocity)},x87_control),x87_abs_max(def(0x1f4),def(0x1f8)),mxcsr);
      const auto r2=guarded_ratio(x87_abs(throttle),x87_abs(def(0x1f4)),mxcsr);
      // F96B6: w = (byte +34C * 0.2 + 1) * 0.5, clamped to [0, 1].
      auto w=mul(add(mul(integer(r8(m,body+0x34cu),mxcsr),fifth_literal,mxcsr),one,mxcsr),half_literal,mxcsr);
      w=clamp_unit(w,mxcsr);
      v=add(mul(sub(one,w,mxcsr),r1,mxcsr),mul(w,r2,mxcsr),mxcsr);
    }
    // F9720: engine_power with 0 >= +25C (COMISS; JB keeps NaN) is 0, inactive.
    if(name==0xc0005a5u) {
      const auto power=fp::sse_compared(field(0x25c),mxcsr);
      if(power<=0.0f){put32(m,value,zero_literal);put8(m,active,0);return 1;}
    }
    // F973B: BL = 1, so the clamped value is published active.
    v=clamp_unit(v,mxcsr);
    put32(m,value,v);
    put8(m,active,1);
    return 1;
  }
  case 0x10000594u: {                                    // F9B45: wingtip_contrail
    // 11FF40 (ECX +70 forward, EDX +88 velocity, EAX and ESI stack locals):
    // the velocity without its forward component.
    const auto f0=field(0x70),f1=field(0x74),f2=field(0x78),v0=field(0x88),v1=field(0x8c),v2=field(0x90);
    const auto dot=add(add(mul(v1,f1,mxcsr),mul(f2,v2,mxcsr),mxcsr),mul(v0,f0,mxcsr),mxcsr);
    const auto p0=mul(f0,dot,mxcsr),p1=mul(dot,f1,mxcsr),p2=mul(f2,dot,mxcsr);
    const auto lateral=x87_length(sub(v0,p0,mxcsr),sub(v1,p1,mxcsr),sub(v2,p2,mxcsr),x87_control);
    const auto ratio=guarded_ratio(lateral,contrail_speed,mxcsr);
    v=mul(ratio,ratio,mxcsr);
    break;
  }
  default:return 0;
  }
  if(clamp)v=clamp_unit(v,mxcsr);
  return publish(m,value,active,v,mxcsr);
}

// ---- 100E70, 10BE20, 11E490: the projectile, device and machine/crate levels ----
namespace {
constexpr std::uint32_t loop_limit=0x3f666666u;         // 0x46A194 (0.9)
constexpr std::uint32_t power_floor=0x38d1b717u;        // 0x46A0EC (1e-4)
constexpr std::uint32_t component_pool=0x52be5cu;       // physics components (0xA0 rows)
constexpr std::uint32_t device_groups=0x4ecbf8u;        // 12-byte rows, +2 flags
// UCOMISS x, 0; LAHF; TEST AH, 0x44; JNP: an ordered x == 0 (DAZ).
bool ordered_zero(std::uint32_t x,std::uint32_t mxcsr){return fp::sse_compared(x,mxcsr)==0.0f;}
// 10C00B / 10C04A: FLD dword a; FABS; FDIV dword d; FSTP dword.
std::uint32_t x87_abs_over(std::uint32_t a,std::uint32_t d,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=fp::x87_control(control);std::uint16_t saved{};std::uint32_t out{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[a]\n\tfabs\n\tfdivs %[d]\n\tfstps %[o]\n\tfldcw %[saved]"
      :[o]"=m"(out),[saved]"+m"(saved):[mode]"m"(mode),[a]"m"(a),[d]"m"(d):"st");
  return out;
#else
  (void)a;(void)d;(void)control;refuse("10BE20","x87 device ratio off x86");
#endif
}
}

std::uint8_t projectile_function_value(M& m,std::uint32_t projectile,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint32_t mxcsr) {
  const auto body=object_body(m,projectile);
  const auto definition=tag_data(m,r32(m,body));
  std::uint32_t v=zero_literal;
  switch(name) {
  case 0xe000562u:v=r32(m,body+0x158u);break;                         // time_remaining
  case 0x6000564u:if((r32(m,body+0x12cu)>>1)&1u)v=one;break;          // tracer
  case 0x11000566u:if((r32(m,body+0x12cu)>>3)&1u)v=one;break;         // projectile_attach
  case 0xf000563u:                                                   // range_remaining
    if(!ordered_zero(r32(m,definition+0xe0u),mxcsr))v=div(r32(m,body+0x168u),r32(m,definition+0xe0u),mxcsr);
    break;
  case 0x12000565u:                                                  // acceleration_range
    if(((r32(m,body+0x12cu)>>8)&1u) || ordered_zero(r32(m,definition+0x17cu),mxcsr) ||
        ordered_zero(r32(m,definition+0x180u),mxcsr))v=one;
    else v=clamp_unit(mul(sub(r32(m,body+0x168u),r32(m,definition+0x18cu),mxcsr),r32(m,definition+0x194u),mxcsr),mxcsr);
    break;
  default:return 0;
  }
  return publish(m,value,active,v,mxcsr);
}

std::uint8_t device_function_value(M& m,std::uint32_t device,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,device);
  const auto definition=tag_data(m,r32(m,body));
  const auto field=[&](std::uint32_t at){return r32(m,body+at);};
  std::uint32_t v=zero_literal;
  bool clamp=true;                                       // 10C06C, else 10C082
  switch(name) {
  case 0x5000561u: {                                     // delay
    const auto d=r32(m,definition+0x10cu);
    if(!above(d,zero_literal,mxcsr)){clamp=false;break;}
    v=mul(div(r32(m,r32(m,game_time)+4u),d,mxcsr),integer(sx16(r16(m,body+0x148u)),mxcsr),mxcsr);
    break;
  }
  case 0x5000081u:v=field(0x134);break;
  case 0x8000080u:v=field(0x140);break;
  case 0x40005abu:                                       // loop: COMISS 0.9, +140; JB (or unordered) is 0
    v=fp::sse_compared(field(0x140),mxcsr)<=f(loop_limit)?one:zero_literal;
    break;
  case 0x6000560u: {                                     // locked
    if(ordered_zero(field(0x134),mxcsr))v=one;
    if(r8(m,body+0xaau)!=7u || field(0x13c)==none)break;
    const auto group=r32(m,r32(m,device_groups)+0x44u)+(field(0x13c)&0xffffu)*12u;
    const auto flags=field(0x1cc);
    if(flags&0xdu)v=one;
    const auto group_flags=r8(m,group+2u);
    if((group_flags&1u) && (group_flags&2u))v=one;
    // 10BF56: +140 == 1.0 (ordered) or +1CC bit 4 publishes 0, unclamped.
    if(fp::sse_compared(field(0x140),mxcsr)==1.0f || (flags&0x10u)){v=zero_literal;clamp=false;}
    break;
  }
  case 0x1200055fu: {                                    // change_in_position
    clamp=false;
    if(ordered_zero(field(0x144),mxcsr))break;
    const auto flags=field(0x12c);
    if(flags&8u) {
      if(flags&0x20u)break;
      const auto speed=field(0x164);
      if(x87_above_abs(power_floor,x87_abs(speed)))break;
      v=x87_abs_quotient(field(0x144),speed,x87_control);
    } else {
      const auto speed=r32(m,definition+0xc8u);
      if(x87_above_abs(power_floor,x87_abs(speed)))break;
      v=x87_abs_over(field(0x144),speed,x87_control);
    }
    clamp=true;
    break;
  }
  case 0xf00055eu: {                                     // change_in_power
    clamp=false;
    if(ordered_zero(field(0x138),mxcsr) || ordered_zero(r32(m,definition+0xc0u),mxcsr))break;
    v=x87_abs_over(field(0x138),r32(m,definition+0xc0u),x87_control);
    clamp=true;
    break;
  }
  default:return 0;
  }
  if(clamp)v=clamp_unit(v,mxcsr);
  return publish(m,value,active,v,mxcsr);
}

std::uint8_t physics_component_function_value(M& m,std::uint32_t component,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint32_t mxcsr) {
  std::uint32_t v=zero_literal;
  switch(name) {
  case 0xa000693u:if(r32(m,component+4u)&0x2000u)v=one;break;        // phantom_on
  case 0xd000691u:v=mul(integer(r8(m,component+0x1bu),mxcsr),byte_unit,mxcsr);break;
  case 0xd000692u:v=mul(integer(r8(m,component+0x1au),mxcsr),byte_unit,mxcsr);break;
  default:return 0;
  }
  return publish(m,value,active,v,mxcsr);
}

std::uint8_t machine_function_value(M& m,std::uint32_t object,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint32_t mxcsr) {
  const auto handle=r32(m,object_body(m,object)+0xb4u);
  if(handle==none)return 0;
  const auto component=r32(m,r32(m,component_pool)+0x44u)+(handle&0xffffu)*0xa0u;
  return physics_component_function_value(m,component,name,value,active,mxcsr)?1:0;
}

namespace {
struct ObjectFunctionSymbol {std::uint32_t instruction,original,mcc,mcc_name;std::string_view name;};
constexpr std::array object_function_symbols{
#include "rendering/effects/source_object_function_symbols_tu15.inc"
};
// Names the translated callbacks match outside the .inc join ({MCC, TU1.5}):
// - the folded SUB chains' targets (BEF80 -> BEF88 -> DEC, C91B2 -> C91B9 ->
//   DEC): scripted_object_function_a/b, driver/gunner_seat_power;
// - the raw-distance links after a joined SUB (the translated SUB keeps the
//   original distance, so it matches the MCC operand plus that distance):
//   BEC50 BEDF6/BEDFD, F9270 F9794/F9986/F9B99/F9D7C, 1046C0
//   104BD8/104C0D/104C12, 1DB800 1DB80D/1DB815;
// - 100E70's projectile names (source_projectile_function_symbol_tu15).
constexpr std::array<std::array<std::uint32_t,2>,20> folded_function_names{{
    {0x1a000729u,0x1a00071au},{0x1a00072au,0x1a00071bu},{0x110005a7u,0x1100059bu},{0x110005a8u,0x1100059cu},
    {0xc0006c3u,0xc0006b7u},{0xc0006c4u,0xc0006b8u},{0xd000592u,0xd000586u},{0xe00058fu,0xe000583u},
    {0x13000594u,0x13000588u},{0x1800059fu,0x18000593u},{0x14000584u,0x14000578u},{0x16000585u,0x16000579u},
    {0x17000577u,0x1700056bu},{0xd00069du,0xd000691u},{0xd00069eu,0xd000692u},
    {0xf00056fu,0xf000563u},{0x6000570u,0x6000564u},{0xe00056eu,0xe000562u},{0x11000572u,0x11000566u},
    {0x12000571u,0x12000565u}}};
thread_local bool mcc_function_names=false;
}

ObjectFunctionNamespace::ObjectFunctionNamespace(bool mcc):previous_(mcc_function_names){mcc_function_names=mcc;}
ObjectFunctionNamespace::~ObjectFunctionNamespace(){mcc_function_names=previous_;}

std::uint32_t object_function_tu15_name(std::uint32_t name) {
  if(!mcc_function_names)return name;
  for(const auto& [mcc,original]:folded_function_names)if(name==mcc)return original;
  bool literal=false;
  for(const auto& symbol:object_function_symbols) {
    if(symbol.original<0x1000000u)continue;  // a folded SUB distance
    if(symbol.mcc_name==name)return symbol.original;
    literal=literal || symbol.original==name;
  }
  for(const auto& [mcc,original]:folded_function_names)literal=literal || original==name;
  return literal?0xffff0000u:name;
}

std::uint8_t object_type_function_value(M& m,std::uint32_t object,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr) {
  name=object_function_tu15_name(name);
  const auto type=s32(sx8(r8(m,object_body(m,object)+0xaau)));
  if(type<0 || type>12)refuse("10DE40","an object type outside the 0x474B90 table");
  put32(m,value,zero_literal);
  put8(m,active,0);
  const auto chain=object_type_chain(std::uint32_t(type));
  const auto callbacks=object_type_callbacks(std::uint32_t(type),0x4cu);
  std::uint8_t accepted=0;
  for(std::size_t level=0;level<chain.size() && chain[level];++level) {
    switch(callbacks[level]) {
    case 0:break;
    case 0xbec50u:accepted=object_base_function_value(m,object,name,value,active,x87_control,mxcsr);break;
    case 0xc9070u:accepted=unit_function_value(m,object,name,value,active,x87_control,mxcsr);break;
    case 0xe0900u:accepted=biped_function_value(m,object,name,value,active,x87_control,mxcsr);break;
    case 0xf9270u:accepted=vehicle_function_value(m,object,name,value,active,x87_control,mxcsr);break;
    case 0x1046c0u:accepted=weapon_function_value(m,object,name,value,active,x87_control,mxcsr);break;
    case 0x100e70u:accepted=projectile_function_value(m,object,name,value,active,mxcsr);break;
    case 0x10be20u:accepted=device_function_value(m,object,name,value,active,x87_control,mxcsr);break;
    case 0x11e490u:accepted=machine_function_value(m,object,name,value,active,mxcsr);break;
    default:refuse("10DE40","an unknown function value callback");
    }
    if(accepted)return accepted;
  }
  put32(m,value,zero_literal);
  put8(m,active,0);
  return 0;
}

std::uint32_t function_reference_value(M& m,std::uint32_t reference,std::uint32_t x,std::uint32_t y,
    std::uint16_t x87_control,std::uint32_t mxcsr) {
  if(!r32(m,reference+4u) || s32(r32(m,reference))<=0)return zero_literal;
  NativeTu15PhysicsFrameFp frame{x87_control,0,mxcsr};
  const auto v=native_tu15_evaluate_authored_function_extended(m,reference,x,y,frame,0);
  return function_map(m,r32(m,reference+4u),v,mxcsr);
}

std::uint8_t object_function_evaluate(M& m,std::uint32_t row,std::uint32_t object,OriginalOutput& value,
    std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto flags=r32(m,row);
  const bool inverted=!((flags>>1)&1u);                  // [ESP+E]
  const bool periodic=r8(m,r32(m,row+0x18u))==3u;        // [ESP+F]
  if(r16(m,object_body(m,object)+0xc0u)&0x200u) {
    OriginalOutput slot{};
    if(object_function_variable(m,object,r32(m,row+4u),value,slot,mxcsr))return 1;
  }
  OriginalOutput import{},import_active{};
  object_type_function_value(m,object,r32(m,row+4u),import,import_active,x87_control,mxcsr);
  auto v=import.value;                                   // [ESP+10]
  auto active=std::uint8_t(import_active.value);         // [ESP+20]
  auto al=active;
  auto current=v;                                        // XMM0
  auto evaluate=[&](std::uint32_t at){return function_reference_value(m,row+0x14u,at,one,x87_control,mxcsr);};
  if(periodic) {
    // BC9FB: the game time phase, optionally offset by the handle draw.
    auto phase=mul(integer(r32(m,r32(m,game_time)+8u),mxcsr),r32(m,r32(m,game_time)+4u),mxcsr);
    if(flags&8u)phase=x87_draw_add(handle_draw(object),phase,x87_control);
    current=mul(evaluate(phase),v,mxcsr);
    v=current;
    al=active;
  } else if(active || inverted) {
    if(flags&1u)v=sub(one,v,mxcsr);
    current=evaluate(v);
    v=current;
    if(inverted)active=above(current,zero_literal,mxcsr)?1:0;
    al=active;
  }
  bool limit=false;
  if(const auto scale=r32(m,row+0x1cu)) {
    OriginalOutput scaled{};
    const auto scale_active=object_function_value(m,object,scale,scaled,x87_control,mxcsr);
    current=mul(scaled.value,v,mxcsr);
    v=current;
    if(active && scale_active){al=active=1;limit=true;}
    else al=active=0;
  } else limit=al!=0;
  if(limit) {
    const auto minimum=r32(m,row+0x10u);
    if(above(minimum,zero_literal,mxcsr))al=active=above(current,minimum,mxcsr)?1:0;
  }
  if(flags&4u)al=active=1;
  if(const auto turn_off=r32(m,row+0xcu); turn_off && al) {
    OriginalOutput off{},off_active{};
    object_type_function_value(m,object,turn_off,off,off_active,x87_control,mxcsr);
    if(!(off_active.value&0xffu)){put32(m,value,zero_literal);return 0;}
    al=active;
    current=v;
  }
  if(!al){put32(m,value,zero_literal);return 0;}
  put32(m,value,clamp_unit(current,mxcsr));
  return al;
}

std::uint8_t object_function_value(M& m,std::uint32_t object,std::uint32_t name,OriginalOutput& value,
    std::uint16_t x87_control,std::uint32_t mxcsr) {
  put32(m,value,zero_literal);
  std::uint8_t result=0;                                 // [ESP+13]
  if(r16(m,object_body(m,object)+0xc0u)&0x200u) {
    OriginalOutput slot{};
    if(object_function_variable(m,object,name,value,slot,mxcsr))return 1;
  }
  const auto literal=object_function_tu15_name(name);    // BCBD9/BCBE5's compiled names
  if(!name || literal==0x30005a6u){put32(m,value,one);return 1;}
  if(literal==0x40005a7u){put32(m,value,zero_literal);return 0;}
  if(object==none)return result;
  OriginalOutput active{};
  if(object_type_function_value(m,object,name,value,active,x87_control,mxcsr))return std::uint8_t(active.value);
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  bool found=false;
  for(std::uint32_t n=0;s32(n)<s32(r32(m,definition+0x64u));++n) {
    const auto row=r32(m,definition+0x68u)+n*0x20u;
    if(r32(m,row+8u)!=name)continue;
    result=object_function_evaluate(m,row,object,value,x87_control,mxcsr);
    found=true;
  }
  if(found)return result;
  if((r32(m,body+4u)>>26)&1u && r32(m,body+0x14u)!=none)
    return object_function_value(m,r32(m,body+0x14u),name,value,x87_control,mxcsr);
  put32(m,value,zero_literal);
  return 0;
}

void contrail_profile_advance(M& m,std::uint32_t contrail) {
  const auto definition=tag_data(m,r32(m,contrail+4u));
  w32(m,contrail+0x28u,zero_literal);
  const auto profiles_tag=r32(m,definition+0x34u);
  if(profiles_tag==none) {
    w16(m,contrail+0x1au,0);
    w16(m,contrail+0x18u,0);
    return;
  }
  const auto profiles=tag_data(m,profiles_tag);
  w16(m,contrail+0x1au,std::uint16_t(r16(m,contrail+0x1au)+1u));
  const auto current=std::int16_t(r16(m,contrail+0x18u));
  const auto point=std::int16_t(r16(m,contrail+0x1au));
  if(current>=0 && s32(std::uint32_t(std::int32_t(current)))<s32(r32(m,profiles+0x3cu)) && point>=0 &&
     point<std::int16_t(r16(m,r32(m,profiles+0x40u)+std::uint32_t(std::int32_t(current))*0x3cu+0x22u)))
    return;
  const auto seed=r32(m,random_seed);
  const auto state=r32(m,seed+4u)*0x19660du+0x3c6ef35fu;
  const auto first=r16(m,definition+0x38u);
  const auto span=std::uint16_t(r16(m,definition+0x3au)+first);
  w32(m,seed+4u,state);
  const auto scaled=(sx16(span)-sx16(first))*(state>>16);
  w16(m,contrail+0x18u,std::uint16_t((scaled>>16)+first));
  w16(m,contrail+0x1au,0);
}

std::uint32_t object_contrail_attachment_create(M& m,std::uint32_t tag,std::uint32_t object,std::uint32_t index,
    std::uint16_t x87_control,std::uint32_t mxcsr) {
  if(tag==none || r32(m,tag_data(m,tag)+0x34u)==none)return none;
  const auto pool=r32(m,contrails);
  const auto handle=native_tu15_data_array_allocate(m,pool);
  if(handle==none)return none;
  const auto row=r32(m,pool+0x44u)+(handle&0xffffu)*0x48u;
  const auto attachment=r32(m,tag_data(m,r32(m,object_body(m,object)))+0x98u)+sx16(std::uint16_t(index))*0x18u;
  w16(m,row+2u,0);
  w32(m,row+4u,tag);
  w32(m,row+8u,object);
  w16(m,row+0xcu,std::uint16_t(index));
  w32(m,row+0x10u,r32(m,attachment+0x10u));
  w16(m,row+0x18u,0xffffu);
  contrail_profile_advance(m,row);
  w32(m,row+0x1cu,zero_literal);
  w32(m,row+0x20u,zero_literal);
  for(std::uint32_t n=0;n<4u;++n) {
    w16(m,row+0x30u+n*2u,0);
    w32(m,row+0x38u+n*4u,none);
  }
  OriginalOutput scale{row+0x14u};
  if(object_function_value(m,r32(m,row+8u),r32(m,row+0x10u),scale,x87_control,mxcsr)) {
    w8(m,row+2u,std::uint8_t(r8(m,row+2u)|1u));
    refuse("17EF70","resetting an active contrail (17E940)");
  }
  return handle;
}

std::uint32_t object_effect_attachment_create(M&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) {
  refuse("178E30","effect_new for an effect attachment (179540)");
}

// ---- Light attachments (C2260) ----
namespace {
constexpr std::uint32_t lights=0x4ecbdcu;              // light data array (0x110 rows)
constexpr std::uint32_t light_epoch=0x4ecbd8u;          // C2260 stores [+0] - 1 at row +C
constexpr std::uint32_t default_color=0x474c70u;        // pointer to the default change color
constexpr std::uint32_t blend_floor=0x38d1b717u;        // 0x46A0EC (1e-4)
constexpr std::uint32_t scale_factor=0x40000000u;       // 0x46A11C (2.0)
constexpr std::uint32_t scale_limit=0x40800000u;        // 0x46A120 (4.0)
std::array<std::uint32_t,3> vector3(const M& m,std::uint32_t at){return {r32(m,at),r32(m,at+4u),r32(m,at+8u)};}
// BCFB0: the root of the object's parent chain (+14), NONE for NONE.
std::uint32_t root_object(const M& m,std::uint32_t object){
  if(object==none)return none;
  auto root=object;
  for(auto parent=r32(m,object_body(m,root)+0x14u);parent!=none;parent=r32(m,object_body(m,root)+0x14u))root=parent;
  return root;
}
// BCD80 with a caller-stack target: the current change color `channel`
// (body +124 bytes / 24 rows from +126) when 0 <= channel < count.
bool object_change_color(const M& m,std::uint32_t object,std::int32_t channel,std::array<std::uint32_t,3>& color){
  const auto body=object_body(m,object);
  const auto base=sx16(r16(m,body+0x126u))+body;
  const auto count=s32((sx16(r16(m,body+0x124u))/12u)>>1);
  if(channel<0 || channel>=count)return false;
  color=vector3(m,base+std::uint32_t(count+channel)*12u);
  return true;
}
// 134730 (EAX flags, ECX a, ESI b, EDI out, XMM5 t): out = a * (1 - t) + b * t.
void color_lerp(M& m,std::uint32_t flags,const std::array<std::uint32_t,3>& a,const std::array<std::uint32_t,3>& b,
    std::uint32_t out,std::uint32_t t,std::uint32_t mxcsr){
  if(flags&1u)refuse("134730","HSV color interpolation (1343E0/134510 on stack colors)");
  const auto u=sub(one,t,mxcsr);
  for(std::uint32_t k=0;k<3u;++k)w32(m,out+4u*k,add(mul(a[k],u,mxcsr),mul(b[k],t,mxcsr),mxcsr));
}
// 134870 on a stack {weight, rgb} pair (C2CB0 always passes weights of 1).
std::array<std::uint32_t,4> weighted(std::uint32_t weight,const std::array<std::uint32_t,3>& c){return {weight,c[0],c[1],c[2]};}
}

void color_interpolate(M& m,std::uint32_t flags,std::uint32_t a,std::uint32_t b,std::uint32_t out,std::uint32_t t,
    std::uint32_t mxcsr) {
  if(flags&1u)refuse("134730","HSV color interpolation (1343E0/134510 on stack colors)");
  const auto u=sub(one,t,mxcsr);
  for(std::uint32_t k=0;k<3u;++k)w32(m,out+4u*k,add(mul(r32(m,a+4u*k),u,mxcsr),mul(r32(m,b+4u*k),t,mxcsr),mxcsr));
}

void light_color_blend(M& m,std::uint32_t out,const std::array<std::uint32_t,4>& a,const std::array<std::uint32_t,4>& b,
    const std::array<std::uint32_t,3>* change_color,std::uint32_t flags,std::uint32_t t,std::uint32_t mxcsr) {
  color_lerp(m,flags,{a[1],a[2],a[3]},{b[1],b[2],b[3]},out,t,mxcsr);
  if(!change_color)return;
  const auto& c=*change_color;
  if(above(a[0],blend_floor,mxcsr) || above(b[0],blend_floor,mxcsr)) {
    const auto w=add(mul(sub(one,t,mxcsr),a[0],mxcsr),mul(b[0],t,mxcsr),mxcsr);
    const auto u=sub(one,w,mxcsr);
    w32(m,out,add(mul(r32(m,out),w,mxcsr),mul(c[0],u,mxcsr),mxcsr));
    w32(m,out+4u,add(mul(c[1],u,mxcsr),mul(r32(m,out+4u),w,mxcsr),mxcsr));
    w32(m,out+8u,add(mul(c[2],u,mxcsr),mul(r32(m,out+8u),w,mxcsr),mxcsr));
    return;
  }
  w32(m,out,mul(r32(m,out),c[0],mxcsr));
  w32(m,out+4u,mul(c[1],r32(m,out+4u),mxcsr));
  w32(m,out+8u,mul(c[2],r32(m,out+8u),mxcsr));
}

namespace {
// ---- 13E6E0: a colour function ----
// A function reference (+0 count, +4 data) evaluated as a 32-bit ARGB
// colour. The data's +0 type byte (1: constant) and +1 flags (bit 0, and
// the high nibble: the colour kind) select it; the colours follow at +4.
struct ColorFunction {
  const M& m;
  std::uint32_t data;
  std::uint8_t type()const{return r8(m,data);}
  std::uint8_t flags()const{return r8(m,data+1u);}
  std::uint32_t kind()const{return flags()>>4;}
  // The colour at data +4 + 4 * index (a NaN index wraps the offset).
  std::uint32_t color(std::uint32_t index)const{return r32(m,data+4u+index*4u);}
};
constexpr std::uint32_t no_color=0xffffffffu;
constexpr std::uint32_t byte_scale=0x437f0000u;      // 0x46A148 (255)
constexpr std::uint32_t blend_scale=0x46800000u;     // 0x48C770 (16384)
constexpr std::uint32_t channel_unit=0x3b808081u;    // 0x46A0C4 (1/255)

// CVTPS2PI's low lane: the nearest integer under the MXCSR rounding, the
// integer indefinite (0x80000000) for NaN or out of range.
std::int32_t sse_round(float value,std::uint32_t mxcsr) {
  const auto mode=fp::sse_control(mxcsr);std::uint32_t saved{};std::int32_t out{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\tcvtss2si %3, %0\n\tldmxcsr %1"
      :"=r"(out),"+m"(saved):"m"(mode),"x"(value));
  return out;
}
// 16AC0: floor to an integer (CVTTSS2SI, less one when the value is
// negative and not whole; the compares under MXCSR.DAZ).
std::int32_t sse_floor(float value,std::uint32_t mxcsr) {
  const auto mode=fp::sse_control(mxcsr);std::uint32_t saved{};std::int32_t whole{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\tcvttss2si %3, %0\n\tldmxcsr %1"
      :"=r"(whole),"+m"(saved):"m"(mode),"x"(value));
  const auto back=fp::sse_compared(bits(fp::cvtsi2ss(whole,mxcsr)),mxcsr),v=fp::sse_compared(bits(value),mxcsr);
  const bool fractional=!(back==v);     // CMPNEQSS: unordered counts
  return fractional && v<0.f?whole-1:whole;
}
// x87: FLD x; FADD ST(0),ST(0) (scale 2) or FMUL qword 3.0 (scale 3); FSTP dword.
float x87_scaled(float x,unsigned scale,std::uint16_t control) {
  const auto mode=fp::x87_control(control);const double three=3.0;std::uint16_t saved{};float out{};
  if(scale==2)asm volatile("fnstcw %[s]\n\tfldcw %[m]\n\tflds %[x]\n\tfadd %%st(0),%%st\n\tfstps %[o]\n\tfldcw %[s]"
      :[o]"=m"(out),[s]"+m"(saved):[m]"m"(mode),[x]"m"(x):"st");
  else asm volatile("fnstcw %[s]\n\tfldcw %[m]\n\tflds %[x]\n\tfmull %[t]\n\tfstps %[o]\n\tfldcw %[s]"
      :[o]"=m"(out),[s]"+m"(saved):[m]"m"(mode),[x]"m"(x),[t]"m"(three):"st");
  return out;
}
// x87: FILD whole; FSUBR value; FSTP dword (the fraction value - whole).
float x87_fraction(float value,std::int32_t whole,std::uint16_t control) {
  const auto mode=fp::x87_control(control);std::uint16_t saved{};float out{};
  asm volatile("fnstcw %[s]\n\tfldcw %[m]\n\tfildl %[w]\n\tfsubrs %[v]\n\tfstps %[o]\n\tfldcw %[s]"
      :[o]"=m"(out),[s]"+m"(saved):[m]"m"(mode),[w]"m"(whole),[v]"m"(value):"st");
  return out;
}
// x87: FILD channel; FMUL colour; FMUL 1/255; FSTP dword.
float x87_channel_scale(std::int32_t channel,float color,std::uint16_t control) {
  const auto mode=fp::x87_control(control);const auto unit=f32(channel_unit);std::uint16_t saved{};float out{};
  asm volatile("fnstcw %[s]\n\tfldcw %[m]\n\tfildl %[c]\n\tfmuls %[k]\n\tfmuls %[u]\n\tfstps %[o]\n\tfldcw %[s]"
      :[o]"=m"(out),[s]"+m"(saved):[m]"m"(mode),[c]"m"(channel),[k]"m"(color),[u]"m"(unit):"st");
  return out;
}
// The colour blend 13E785..13E7E7 (with its two copies): per channel in
// 16-bit lanes, c0 + ((((c1 - c0) << 3) * t >> 16) + 1 >> 1) saturated to a
// byte (PSUBW, PSLLW, PMULHW, PADDW, PSRAW, PADDW, PACKUSWB), where t is the
// fraction times 16384 rounded by CVTPS2PI (its low word, broadcast).
std::uint32_t color_blend(std::uint32_t c0,std::uint32_t c1,float fraction,std::uint32_t mxcsr) {
  const auto t=std::int16_t(sse_round(fp::mulss(fraction,f32(blend_scale),mxcsr),mxcsr));
  std::uint32_t out=0;
  for(unsigned lane=0;lane<4u;++lane) {
    const auto a=std::int16_t((c0>>(8u*lane))&0xffu),b=std::int16_t((c1>>(8u*lane))&0xffu);
    const auto step=std::int16_t(std::uint16_t(std::int16_t(b-a))<<3);
    auto delta=std::int16_t((std::int32_t(step)*t)>>16);
    delta=std::int16_t(std::int16_t(delta+1)>>1);
    const auto v=std::int16_t(a+delta);
    out|=std::uint32_t(v<0?0:v>255?255:v)<<(8u*lane);
  }
  return out;
}
}

std::uint32_t color_function_value(const M& m,std::uint32_t reference,std::uint32_t input,std::uint16_t x87_control,
    std::uint32_t mxcsr) {
  const ColorFunction function{m,r32(m,reference+4u)};
  if(!function.data || s32(r32(m,reference))<=0)return no_color;
  if(function.type()==1u && (!(function.flags()&1u) || (function.flags()&0xf0u)<=0x10u))return function.color(0);
  // The input clamped to [0, 1] (COMISS; NaN is kept).
  auto x=f32(input);
  if(above(zero_literal,bits(x),mxcsr))x=0.f;
  else if(above(bits(x),one,mxcsr))x=f32(one);
  switch(function.kind()) {
  case 0: {  // grey: the input times 255 (FISTP under the x87 control word), opaque
    const auto level=fp::fistp32(fp::mulss(x,f32(byte_scale),mxcsr),x87_control)&0xffu;
    return 0xff000000u|level<<16|level<<8|level;
  }
  case 1:return function.color(0);
  case 2:return color_blend(function.color(0),function.color(3),x,mxcsr);
  case 3: {  // three stops: +4, +8 and +10 over two halves
    const auto scaled=x87_scaled(x,2,x87_control);
    const auto whole=sse_floor(scaled,mxcsr);
    auto fraction=x87_fraction(scaled,whole,x87_control);
    if(whole>=2)fraction=f32(one);
    const bool first=whole<2 && whole==0;
    return color_blend(function.color(first?0u:1u),function.color(first?1u:3u),fraction,mxcsr);
  }
  case 4: {  // four stops over three thirds
    const auto scaled=x87_scaled(x,3,x87_control);
    auto whole=sse_floor(scaled,mxcsr);
    auto fraction=x87_fraction(scaled,whole,x87_control);
    if(whole>=3){fraction=f32(one);whole=2;}
    return color_blend(function.color(std::uint32_t(whole)),function.color(std::uint32_t(whole)+1u),fraction,mxcsr);
  }
  default:return no_color;
  }
}

void light_function_scales(M& m,std::uint32_t light,std::uint32_t intensity,std::uint32_t colors,std::uint32_t functions,
    std::uint32_t scale,std::uint32_t pair,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto time=r32(m,game_time);
  std::uint32_t phase;
  if(r8(m,functions)&1u) {
    phase=mul(integer(r32(m,time+8u),mxcsr),r32(m,time+4u),mxcsr);
  } else {
    const auto period=s32(sx16(r16(m,time+2u))*0x3cu);
    if(!period)refuse("C2B30","a zero game tick rate (IDIV divide fault)");
    const auto offset=std::uint32_t(s32((light*0x1387u)&0x7fffffffu)%period);
    phase=mul(integer(offset+r32(m,time+8u),mxcsr),r32(m,time+4u),mxcsr);
  }
  if(s32(r32(m,functions+4u))>0) {
    const auto reference=r32(m,functions+8u);
    NativeTu15PhysicsFrameFp frame{x87_control,0,mxcsr};
    auto v=native_tu15_evaluate_authored_function_extended(m,reference,phase,intensity,frame,0);
    v=function_map(m,r32(m,reference+4u),v,mxcsr);
    w32(m,scale,mul(r32(m,scale),v,mxcsr));
  }
  if(s32(r32(m,functions+0xcu))>0) {
    // C2BED: the colour function (13DE70 for its input, 13E6E0) tints the
    // light's first colour: each channel (red, green, blue) times its byte
    // over 255, in x87.
    const auto reference=r32(m,functions+0x10u);
    NativeTu15PhysicsFrameFp frame{x87_control,0,mxcsr};
    const auto input=native_tu15_evaluate_authored_function_extended(m,reference,phase,intensity,frame,0);
    const auto color=color_function_value(m,reference,input,x87_control,mxcsr);
    for(std::uint32_t k=0;k<3u;++k) {
      const auto channel=std::int32_t((color>>(16u-8u*k))&0xffu);
      w32(m,colors+4u*k,bits(x87_channel_scale(channel,f32(r32(m,colors+4u*k)),x87_control)));
    }
  }
  if(s32(r32(m,functions+0x14u))>0) {
    const auto references=r32(m,functions+0x18u);
    w32(m,pair,function_reference_value(m,references,phase,intensity,x87_control,mxcsr));
    w32(m,pair+4u,function_reference_value(m,references+8u,phase,intensity,x87_control,mxcsr));
  }
}

void light_update(M& m,std::uint32_t light,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto row=r32(m,r32(m,lights)+0x44u)+(light&0xffffu)*0x110u;
  const auto definition=tag_data(m,r32(m,row+4u));
  std::uint32_t owner=none,body=0;
  if(const auto object=r32(m,row+0x4cu);object!=none){owner=object;body=object_body(m,object);}
  else if(const auto parent=r32(m,row+0x58u);parent!=none){owner=parent;body=object_body(m,parent);}
  const auto flags=r32(m,definition+0x34u);
  if(r16(m,row+0x54u)!=0xffffu) {
    // C2D4F: stack copies of the default color and the four definition colors.
    auto change=vector3(m,r32(m,default_color));
    const auto first_near=weighted(one,vector3(m,definition+0x58u)),first_far=weighted(one,vector3(m,definition+0x64u));
    const auto second_near=weighted(one,vector3(m,definition+0x40u)),second_far=weighted(one,vector3(m,definition+0x4cu));
    OriginalOutput intensity{row+0xd0u};
    object_function_value(m,owner,r32(m,row+0x50u),intensity,x87_control,mxcsr);
    object_change_color(m,owner,s32(sx16(r16(m,row+0x56u))),change);
    light_color_blend(m,row+0xd4u,first_near,first_far,&change,flags,r32(m,row+0xd0u),mxcsr);
    light_color_blend(m,row+0xe0u,second_near,second_far,&change,flags,r32(m,row+0xd0u),mxcsr);
    const auto level=r32(m,row+0xd0u);         // C2E4C / C2F0B: the scale blend
    w32(m,row+0xecu,add(mul(sub(one,level,mxcsr),r32(m,definition+0x70u),mxcsr),
        mul(r32(m,definition+0x74u),level,mxcsr),mxcsr));
  } else if(r8(m,row+2u)&0x10u) {
    const auto time=r32(m,game_time);
    const auto elapsed=r32(m,time+8u)-r32(m,row+0x14u);
    const auto rate=mul(integer(sx16(r16(m,time+2u)),mxcsr),r32(m,definition+0xb0u),mxcsr);
    const auto fraction=div(integer(elapsed,mxcsr),rate,mxcsr);
    NativeTu15PhysicsFrameFp frame{x87_control,0,mxcsr};
    const auto fade=native_tu15_evaluate_transition_curve(m,r16(m,definition+0xb6u),fraction,frame,0);
    w32(m,row+0xd0u,mul(sub(one,fade,mxcsr),r32(m,row+0xccu),mxcsr));
    const auto t=r32(m,row+0xd0u);
    color_lerp(m,flags,vector3(m,definition+0x58u),vector3(m,definition+0x64u),row+0xd4u,t,mxcsr);
    color_lerp(m,flags,vector3(m,definition+0x40u),vector3(m,definition+0x4cu),row+0xe0u,r32(m,row+0xd0u),mxcsr);
    const auto level=r32(m,row+0xd0u);         // C2E4C / C2F0B: the scale blend
    w32(m,row+0xecu,add(mul(sub(one,level,mxcsr),r32(m,definition+0x70u),mxcsr),
        mul(r32(m,definition+0x74u),level,mxcsr),mxcsr));
  } else {
    w32(m,row+0xd0u,r32(m,row+0xccu));
    for(std::uint32_t k=0;k<3u;++k)w32(m,row+0xd4u+4u*k,r32(m,definition+0x64u+4u*k));
    for(std::uint32_t k=0;k<3u;++k)w32(m,row+0xe0u+4u*k,r32(m,definition+0x4cu+4u*k));
    w32(m,row+0xecu,r32(m,definition+0x74u));
  }
  light_function_scales(m,light,r32(m,row+0xd0u),row+0xd4u,definition+0xc0u,row+0xecu,row+0xf0u,x87_control,mxcsr);
  w32(m,row+0xc8u,zero_literal);
  if(body) {
    const auto root=object_body(m,root_object(m,owner));
    if(((1u<<(r8(m,root+0xaau)&31u))&3u) && above(r32(m,root+0x2b0u),zero_literal,mxcsr) &&
       !(r8(m,tag_data(m,r32(m,row+4u)))&0x80u)) {
      auto camouflage=r32(m,root+0x2b0u);
      if(above(zero_literal,camouflage,mxcsr))camouflage=zero_literal;
      else if(above(camouflage,one,mxcsr))camouflage=one;
      w32(m,row+0xc8u,camouflage);
      const auto keep=sub(one,camouflage,mxcsr);
      for(std::uint32_t at=0xd4u;at<=0xe0u;at+=4u)w32(m,row+at,mul(r32(m,row+at),keep,mxcsr));
      w32(m,row+0xe4u,mul(keep,r32(m,row+0xe4u),mxcsr));
      w32(m,row+0xe8u,mul(keep,r32(m,row+0xe8u),mxcsr));
    }
  }
  std::array<std::uint32_t,6> clamped{};
  for(std::uint32_t k=0;k<6u;++k) {
    clamped[k]=clamp_unit(r32(m,row+0xd4u+4u*k),mxcsr);
    w32(m,row+0xd4u+4u*k,clamped[k]);
  }
  auto scaled=mul(r32(m,row+0xecu),scale_factor,mxcsr);
  if(above(zero_literal,scaled,mxcsr))scaled=zero_literal;
  else if(above(scaled,scale_limit,mxcsr))scaled=scale_limit;
  const auto state=r16(m,row+2u);
  w32(m,row+0xecu,scaled);
  const auto radius=add(mul(sub(one,r32(m,row+0xd0u),mxcsr),r32(m,definition+8u),mxcsr),
      mul(r32(m,definition+0xcu),r32(m,row+0xd0u),mxcsr),mxcsr);
  w32(m,row+0xc4u,radius);
  bool visible=false;
  for(const auto c:clamped)visible=visible || !(fp::sse_compared(c,mxcsr)==0.0f);
  if((state&1u) && above(radius,zero_literal,mxcsr) && visible) {
    if(state&2u)return;
    w16(m,row+2u,std::uint16_t(state|2u));
    if(owner!=none) {
      if(native_tu15_object_or_ancestor_deleting(m,owner))return;
      if(!native_tu15_object_root_attached(m,owner))return;
    }
    const auto globals=r32(m,object_globals);
    if(!globals || !r8(m,globals))return;
    NativeTu15PhysicsFrameFp frame{x87_control,0,mxcsr};
    light_reconnect(m,light,frame);
    return;
  }
  if(!(state&2u))return;
  if(state&8u)native_tu15_detach_light_attachment(m,light);
  w8(m,row+2u,std::uint8_t(r8(m,row+2u)&0xfdu));
}

std::uint32_t object_light_attachment_create(M& m,std::uint32_t tag,std::uint32_t object,std::uint32_t index,
    std::uint32_t primary_scale,std::uint32_t marker,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto definition=tag_data(m,tag);
  const auto mode=r32(m,r32(m,player_mode)+8u);
  const bool forced=mode==5u || mode==4u;
  const bool split=r16(m,definition+0xa4u)==2u;
  if((r8(m,definition)&1u) && r32(m,definition+0x94u)==none)return none;
  if(!forced && split)return none;
  const auto pool=r32(m,lights);
  const auto light=native_tu15_data_array_allocate(m,pool);
  if(light==none)return none;
  const auto row=r32(m,pool+0x44u)+(light&0xffffu)*0x110u;
  w32(m,row+4u,tag);
  w32(m,row+0x4cu,object);
  w16(m,row+2u,0);
  w16(m,row+0x54u,std::uint16_t(index));
  w32(m,row+0x50u,primary_scale);
  w32(m,row+0x14u,none);
  w32(m,row+8u,none);
  w16(m,row+0x56u,std::uint16_t(marker));
  w32(m,row+0x58u,none);
  w16(m,row+0x5cu,0xffffu);
  w32(m,row+0xd0u,one);
  w32(m,row+0xccu,one);
  w16(m,row+2u,std::uint16_t(~r8(m,definition)&1u));
  w8(m,row+2u,std::uint8_t(r8(m,row+2u)&0xefu));
  w32(m,row+0x10u,none);
  light_update(m,light,x87_control,mxcsr);
  w32(m,row+0xcu,r32(m,light_epoch)-1u);
  w32(m,row+0x10cu,none);
  return light;
}
}
