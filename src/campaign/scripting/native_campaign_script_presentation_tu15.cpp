#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "native_campaign_script_fp_tu15.hpp"
#include <array>
#include <bit>
#include <cmath>

// Native replications of small TU1.5 presentation/object-state owners that
// campaign builtins previously reached through original-code execution.
namespace halo2::native_campaign_script_tu15 {
namespace {
using Bank=NativeCampaignScriptBank;
using namespace native_campaign_script_fp;
constexpr std::uint32_t object_pool_global=0x4ecbd0u;
float as_float(std::uint32_t bits){return std::bit_cast<float>(bits);}
std::uint32_t as_bits(float value){return std::bit_cast<std::uint32_t>(value);}
std::uint32_t object_body(const Bank& bank,std::uint32_t object) {
  const auto records=bank.u32(bank.u32(object_pool_global)+0x44u);
  return bank.u32(records+(object&0xffffu)*12u+8u);
}
std::uint64_t u64(const Bank& bank,std::uint32_t address) {
  return std::uint64_t(bank.u32(address))|(std::uint64_t(bank.u32(address+4u))<<32u);
}
}

void object_cinematic_lod(Bank& bank,std::uint32_t object,std::uint8_t active) {
  if(object==none)return;
  const auto flags=object_body(bank,object)+4u;
  const auto value=bank.u32(flags);
  bank.store32(flags,active?value|0x80000u:value&0xfff7ffffu);
}
void unit_set_enterable_by_player(Bank& bank,std::uint32_t unit,std::uint8_t enabled) {
  if(unit==none)return;
  const auto flags=object_body(bank,unit)+0x134u;
  const auto value=bank.u32(flags);
  bank.store32(flags,enabled?value&0xffffefffu:value|0x1000u);
}
void object_clear_function_variable(Bank& bank,std::uint32_t object,std::uint32_t name) {
  if(object==none || !name || name==none)return;
  const auto slots=bank.u32(0x51daf4u);
  for(unsigned slot=0;slot<32;++slot)
    if(bank.u32(slots+0x280u+slot*4u)==object && bank.u32(slots+slot*0x14u)==name)
      bank.store32(slots+0x280u+slot*4u,none);
}
void texture_camera_off(Bank& bank) {
  for(unsigned n=0;n<12;++n)bank.store32(0x4c6190u+n*4u,0);
  bank.store32(0x4c6190u,none);
  bank.store8(0x515edcu,0);
}
void cinematic_shadow_control(Bank& bank,std::uint8_t enabled,std::uint32_t object,
    std::uint32_t marker,std::uint32_t radius) {
  constexpr std::uint32_t table=0x4c6894u,count_global=0x4c6954u;
  if(object==none)return;
  const auto count=std::int32_t(bank.u32(count_global));
  std::int32_t index=0;
  for(;index<count;++index)if(bank.u32(table+std::uint32_t(index)*12u)==object)break;
  if(index>=count || count<=0) {
    if(!enabled || std::uint32_t(count)>=16u)return;
    const auto entry=table+std::uint32_t(count)*12u;
    bank.store32(count_global,std::uint32_t(count+1));
    bank.store32(entry,object);
    bank.store32(entry+4u,marker);
    bank.store32(entry+8u,radius);
    return;
  }
  const auto entry=table+std::uint32_t(index)*12u;
  if(enabled) {
    bank.store32(entry+4u,marker);
    bank.store32(entry+8u,radius);
    return;
  }
  // Removal moves the last row into the vacated slot.
  const auto last=count-1;
  bank.store32(count_global,std::uint32_t(last));
  if(index>=last)return;
  const auto source=table+std::uint32_t(last)*12u;
  bank.store32(entry,bank.u32(source));
  bank.store32(entry+4u,bank.u32(source+4u));
  bank.store32(entry+8u,bank.u32(source+8u));
}
void cinematic_depth_of_field(Bank& bank,std::span<const std::uint32_t,7> a) {
  const auto screen=bank.u32(0x515ec0u);
  if(!screen)return;
  const auto& fp=bank.floating_point();
  auto focus=a[0];
  if(sse_compared(0,fp.mxcsr)>sse_compared(focus,fp.mxcsr))focus=0;
  bank.store32(screen+0xd8u,focus);
  const auto ramp=[&](std::uint32_t first,std::uint32_t second,std::uint32_t seconds,std::uint32_t at) {
    if(!(sse_compared(seconds,fp.mxcsr)>=0.f))return;
    const auto [now,end]=x87_clock_store_add(std::bit_cast<double>(u64(bank,0x4920c0u)),as_float(seconds),fp.x87_control_word);
    bank.store32(screen+at+8u,as_bits(now));
    bank.store32(screen+at,first);
    bank.store32(screen+at+4u,second);
    bank.store32(screen+at+0xcu,as_bits(end));
  };
  ramp(a[1],a[2],a[3],0xdcu);
  ramp(a[4],a[5],a[6],0xecu);
}
void camera_set_field_of_view(Bank& bank,std::uint32_t degrees,std::uint16_t ticks) {
  const auto& fp=bank.floating_point();const auto mx=fp.mxcsr;
  const auto director=bank.u32(0x51df6cu);
  const auto initial=bank.u32(0x55d6e4u);
  // UCOMISS equality: ordered and equal (DAZ applies to the operand).
  if(sse_compared(bank.u32(director+0xcu),mx)==0.f)bank.store32(director+0xcu,initial);
  auto value=initial;
  const auto cinematic=bank.u32(0x51df50u);
  if(cinematic && bank.u8(cinematic+5u)) {
    const auto target=bank.u32(director+0x10u);
    if(!(sse_compared(target,mx)==0.f)) {
      const auto elapsed=bank.u32(director+4u),total=bank.u32(director+8u);
      if(!(sse_compared(elapsed,mx)<sse_compared(total,mx)) &&
          !std::isnan(sse_compared(elapsed,mx)) && !std::isnan(sse_compared(total,mx)))value=target;
      else {
        auto fraction=as_bits(divss(as_float(elapsed),as_float(total),mx));
        if(0.f>sse_compared(fraction,mx))fraction=0;
        else if(sse_compared(fraction,mx)>1.f)fraction=0x3f800000u;
        const auto current=as_float(bank.u32(director+0xcu));
        value=as_bits(addss(mulss(subss(as_float(target),current,mx),as_float(fraction),mx),current,mx));
      }
    }
  }
  bank.store32(director+0xcu,value);
  bank.store32(director+8u,as_bits(mulss(cvtsi2ss(std::int16_t(ticks),mx),as_float(0x3d088889u),mx)));
  const auto radians=mulss(as_float(degrees),as_float(0x3c8efa35u),mx);
  bank.store32(director+0x10u,as_bits(radians));
  bank.store32(director+4u,0);
  const auto bits=as_bits(radians);
  const bool admitted=(bits&0x7f800000u)!=0x7f800000u &&
      !(sse_compared(bits,mx)<sse_compared(bank.u32(0x55d6e8u),mx) || std::isnan(sse_compared(bank.u32(0x55d6e8u),mx))) &&
      sse_compared(bank.u32(0x55d6ecu),mx)>=sse_compared(bits,mx);
  if(admitted)return;
  for(unsigned offset=4;offset<=0x10u;offset+=4)bank.store32(director+offset,0);
}
void cinematic_stop(Bank& bank) {
  const auto collection=bank.u32(0x4eabc4u),users=bank.u32(0x4f54f0u);
  bank.store8(collection+0x81u,0);
  bank.store8(collection+0x80u,0);
  bank.store8(bank.u32(0x4f3218u)+0x11f8u,0);
  const auto cinematic=bank.u32(0x51df50u);
  bank.store8(cinematic+4u,0);
  bank.store8(users+6u,0);
  const auto globals=bank.u32(0x5020a8u);
  const auto screen=bank.u32(0x515ec0u);
  bank.store8(globals+0x20u,1);
  bank.store8(cinematic+5u,0);
  if(screen) {
    for(unsigned n=0;n<0xffu;++n)bank.store32(screen+n*4u,0);
    for(const auto offset:{0x180u,0x184u,0x188u,0x18cu,0x3f8u})bank.store32(screen+offset,0x3f800000u);
    bank.store32(screen+0x190u,0);
  }
  bank.store8(bank.u32(0x515e2cu),0);
  const auto director=bank.u32(0x51df6cu)+4u;
  for(unsigned n=0;n<4;++n)bank.store32(director+n*4u,0);
  bank.store8(0x515ef5u,0);
}

std::optional<NativeCampaignFunctionValue> object_function_value(const Bank& bank,
    std::uint32_t object,std::uint32_t name) {
  if(object==none)return std::nullopt;
  const auto slots=bank.u32(0x51daf4u);
  for(std::uint32_t slot=0;slot<32;++slot) {
    if(bank.u32(slots+0x280u+slot*4u)!=object || bank.u32(slots+slot*0x14u)!=name)continue;
    const auto entry=slots+slot*0x14u;
    const auto duration=std::int32_t(bank.u32(entry+0x10u));
    const auto start=bank.u32(entry+4u),target=bank.u32(entry+8u);
    const auto elapsed=std::int32_t(bank.u32(bank.u32(0x51df54u)+8u)-bank.u32(entry+0xcu));
    if(elapsed>=duration || !duration)return NativeCampaignFunctionValue{target,slot};
    const auto mx=bank.floating_point().mxcsr;
    const auto fraction=divss(cvtsi2ss(elapsed,mx),cvtsi2ss(duration,mx),mx);
    const auto value=addss(mulss(fraction,subss(as_float(target),as_float(start),mx),mx),as_float(start),mx);
    return NativeCampaignFunctionValue{as_bits(value),slot};
  }
  return std::nullopt;
}
void object_set_function_variable(Bank& bank,std::uint32_t object,std::uint32_t name,
    std::uint32_t value,std::uint32_t frames) {
  const auto& fp=bank.floating_point();const auto mx=fp.mxcsr;
  const auto seconds=mulss(as_float(frames),as_float(0x3d088889u),mx);
  if(object==none || !name || name==none)return;
  const auto current=object_function_value(bank,object,name);
  const auto start=current?current->value:value;
  const auto slots=bank.u32(0x51daf4u);
  std::uint32_t slot;
  if(current)slot=current->slot;
  else {
    // First free row, otherwise the row with the oldest signed start time.
    std::int32_t oldest=0x7fffffff;std::uint32_t chosen=none;bool found=false;
    for(std::uint32_t n=0;n<32;++n) {
      if(bank.u32(slots+0x280u+n*4u)==none){slot=n;found=true;break;}
      const auto started=std::int32_t(bank.u32(slots+n*0x14u+0xcu));
      if(started<oldest){oldest=started;chosen=n;}
    }
    if(!found)slot=chosen;
  }
  const auto body=object_body(bank,object);
  bank.store32(slots+slot*4u+0x280u,object);
  const auto entry=slots+slot*0x14u;
  bank.store32(entry,name);
  bank.store32(entry+4u,start);
  bank.store32(entry+8u,value);
  const auto time=bank.u32(0x51df54u);
  bank.store32(entry+0xcu,bank.u32(time+8u));
  const auto clamped=0.f>sse_compared(as_bits(seconds),mx)?0.f:seconds;
  const auto rate=std::int16_t(bank.u16(time+2u));
  bank.store32(entry+0x10u,fistp32(mulss(cvtsi2ss(rate,mx),clamped,mx),fp.x87_control_word));
  bank.store8(body+0xc1u,std::uint8_t(bank.u8(body+0xc1u)|2u));
}
namespace {
// CVTTSS2SI: truncation; NaN and out-of-range give integer indefinite.
std::uint32_t cvttss2si(float value) {
  if(std::isnan(value) || value>=2147483648.f || value< -2147483648.f)return 0x80000000u;
  return std::uint32_t(std::int32_t(value));
}
std::uint8_t saturated_byte(std::uint32_t value){return value>0xffu?0xffu:std::uint8_t(value);}
// 44A9A4: TU1.5 emotion name ids in row order (.rdata, immutable).
constexpr std::array<std::uint32_t,13> emotion_names{0x6000084u,0x70001a4u,0x50001a5u,0xb0001a6u,0x80001a7u,
    0xc0001a8u,0x70001a9u,0x60001aau,0x80001abu,0x70001acu,0x50001adu,0x70001aeu,0x40001afu};
}
void unit_set_emotion(Bank& bank,std::uint32_t unit,std::uint32_t emotion,std::uint32_t weight,std::uint32_t seconds) {
  std::int16_t index=-1;
  if(emotion!=0x7000001u)
    for(std::int16_t n=0;n<13;++n)if(emotion==emotion_names[std::size_t(n)]){index=n;break;}
  const auto body=object_body(bank,unit);
  const auto face=body+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x33eu))));
  if((bank.u32(face+0x5cu)>>16u)==0xffffu)return;
  if(index!=-1 && index<13) {
    bank.store16(face+0x6cu,std::uint16_t(index));
    bank.store32(face+0x68u,weight);
  } else {
    bank.store16(face+0x6cu,0xffffu);
    bank.store32(face+0x68u,0);
  }
  const auto mx=bank.floating_point().mxcsr;
  if(sse_compared(seconds,mx)>0.f) {
    const auto rate=std::int16_t(bank.u16(bank.u32(0x51df54u)+2u));
    const auto ticks=mulss(cvtsi2ss(rate,mx),as_float(seconds),mx);
    bank.store32(face+0x60u,as_bits(divss(1.f,ticks,mx)));
    bank.store32(face+0x64u,0);
    return;
  }
  for(unsigned n=0;n<13;++n)bank.store8(face+0x6eu+n,0);
  const auto scaled=mulss(as_float(bank.u32(face+0x68u)),128.f,mx);
  bank.store8(face+0x6eu+std::uint32_t(std::int32_t(index)),saturated_byte(cvttss2si(scaled)));
  bank.store32(face+0x60u,0);
  bank.store32(face+0x64u,0);
}

namespace {
// 10CB60 over the 0x20-byte animation state at `state`. start_velocity is the
// caller's XMM6; the end velocity argument is always 0 in TU1.5 callers.
void device_velocity_profile(Bank& bank,std::uint32_t state,std::uint32_t position,std::uint32_t seconds,
    std::uint32_t acceleration,std::uint32_t deceleration,std::uint32_t start_velocity) {
  const auto mx=bank.floating_point().mxcsr;
  const auto c=[mx](std::uint32_t bits){return sse_compared(bits,mx);};
  constexpr std::uint32_t one=0x3f800000u,end_velocity=0;
  auto time=seconds,accel=acceleration,decel=deceleration;
  if(!(c(time)>0.f))time=0;
  if(!(c(accel)>0.f))accel=0;
  if(!(c(decel)>0.f))decel=0;
  if(c(accel)>c(time))accel=time;
  const auto coast_limit=as_bits(subss(as_float(time),as_float(accel),mx));
  if(!(c(coast_limit)>c(decel)))decel=coast_limit;
  const auto stored=bank.u32(state);
  std::uint32_t current;
  if(0.f>c(stored))current=0;
  else current=c(stored)>1.f?one:stored;
  std::uint32_t target=position;
  if(0.f>c(target))target=0;
  else if(c(target)>1.f)target=one;
  const auto delta=as_bits(subss(as_float(target),as_float(current),mx));
  bank.store32(state,target);
  bank.store32(state+8u,target);
  bank.store32(state+0xcu,target);
  bank.store32(state+0x10u,0);
  bank.store32(state+0x14u,0);
  bank.store32(state+4u,0);
  bank.store32(state+0x18u,0);
  bank.store32(state+0x1cu,time);
  // x87 FABS/FCOMI against the original 1e-4 and 1e-6 thresholds.
  if(as_float(0x38d1b717u)>std::fabs(as_float(time)))return;
  const auto distance=std::fabs(as_float(delta));
  if(as_float(0x358637bdu)>distance)return;
  const auto coast=subss(as_float(coast_limit),as_float(decel),mx);
  bank.store32(state+0xcu,target);
  bank.store32(state,current);
  bank.store32(state+8u,current);
  const auto half_decel=mulss(as_float(decel),0.5f,mx);
  const auto half_accel=mulss(as_float(accel),0.5f,mx);
  const auto ramp=addss(addss(coast,half_decel,mx),half_accel,mx);
  bank.store32(state+0x10u,0);
  bank.store32(state+0x14u,as_bits(distance));
  bank.store32(state+4u,start_velocity);
  bank.store32(state+0x18u,0);
  float cruise;
  if(c(as_bits(ramp))==0.f && !std::isnan(c(as_bits(ramp))))
    cruise=divss(as_float(delta),as_float(time),mx);
  else {
    auto remaining=subss(as_float(delta),mulss(half_accel,as_float(start_velocity),mx),mx);
    remaining=subss(remaining,mulss(half_decel,as_float(end_velocity),mx),mx);
    cruise=divss(remaining,ramp,mx);
  }
  bank.store32(state+0x18u,as_bits(cruise));
  if(c(accel)>0.f) {
    const auto rate=divss(subss(cruise,as_float(start_velocity),mx),as_float(accel),mx);
    if(!(c(as_bits(rate))==0.f && !std::isnan(c(as_bits(rate))))) {
      const auto squares=subss(mulss(cruise,cruise,mx),mulss(as_float(start_velocity),as_float(start_velocity),mx),mx);
      bank.store32(state+0x10u,as_bits(divss(squares,mulss(rate,2.f,mx),mx)));
    }
  }
  if(c(decel)>0.f) {
    const auto velocity=as_float(bank.u32(state+0x18u));
    const auto rate=divss(subss(as_float(end_velocity),velocity,mx),as_float(decel),mx);
    if(!(c(as_bits(rate))==0.f && !std::isnan(c(as_bits(rate))))) {
      const auto squares=subss(mulss(as_float(end_velocity),as_float(end_velocity),mx),mulss(velocity,velocity,mx),mx);
      const auto braking=divss(squares,mulss(rate,2.f,mx),mx);
      bank.store32(state+0x14u,as_bits(subss(distance,braking,mx)));
    }
  }
}
}
void device_animate_position(Bank& bank,std::uint32_t device,std::uint32_t position,std::uint32_t seconds,
    std::uint32_t acceleration,std::uint32_t deceleration,std::uint8_t keep_velocity) {
  if(device==none)return;
  const auto body=object_try_and_get_and_verify_type(bank,device,0x380u);
  if(!body)return;
  const auto flags=bank.u32(body+0x12cu);
  if(!(flags&8u))return;
  bank.store32(body+0x12cu,flags&0xffffffdfu);
  const auto velocity=keep_velocity?bank.u32(body+0x150u):0u;
  device_velocity_profile(bank,body+0x14cu,position,seconds,acceleration,deceleration,velocity);
  object_mark_for_update(bank,device);
}
void device_animate_overlay(Bank& bank,std::uint32_t device,std::uint32_t position,std::uint32_t seconds,
    std::uint32_t acceleration,std::uint32_t deceleration) {
  if(device==none)return;
  const auto body=object_try_and_get_and_verify_type(bank,device,0x380u);
  if(!body)return;
  const auto flags=bank.u32(body+0x12cu);
  if(!(flags&0x10u))return;
  bank.store32(body+0x12cu,flags&0xffffffbfu);
  device_velocity_profile(bank,object_body(bank,device)+0x16cu,position,seconds,acceleration,deceleration,0);
  object_mark_for_update(bank,device);
}

void original_strncpy(Bank& bank,std::uint32_t destination,std::uint32_t source,std::uint32_t count) {
  std::uint32_t dst=destination,src=source,remaining=count,tail=count;
  if(!remaining)return;
  // Shared zero padding (32A54E/32A5E7/32A573): byte-align, words, bytes.
  const auto pad=[&](std::uint32_t left,bool aligned_words) {
    if(!aligned_words) {
      while(dst&3u){bank.store8(dst++,0);if(!--left)return;}
    }
    auto words=left>>2u;const auto bytes=left&3u;
    while(words--){bank.store32(dst,0);dst+=4u;}
    for(auto n=bytes;n;--n)bank.store8(dst++,0);
  };
  // 32A533: trailing bytes; a NUL copies then zero-fills the rest.
  const auto bytes=[&](std::uint32_t left) {
    while(left) {
      const auto value=bank.u8(src++);bank.store8(dst++,value);
      if(!value){for(--left;left;--left)bank.store8(dst++,0);return;}
      --left;
    }
  };
  if(src&3u) {
    for(;;) {
      const auto value=bank.u8(src++);bank.store8(dst++,value);
      if(!--remaining)return;
      if(!value){pad(remaining,false);return;}
      if(!(src&3u))break;
    }
    tail=remaining;
  }
  auto words=remaining>>2u;
  if(!words){bytes(tail);return;}
  tail=remaining&3u;
  while(words) {
    const auto word=bank.u32(src);src+=4u;
    const bool maybe_zero=((~word)^(word+0x7efefeffu))&0x81010100u;
    std::uint32_t stored=word;bool zero=false;
    if(maybe_zero) {
      if(!(word&0xffu)){stored=0;zero=true;}
      else if(!(word&0xff00u)){stored=word&0xffu;zero=true;}
      else if(!(word&0xff0000u)){stored=word&0xffffu;zero=true;}
      else if(!(word&0xff000000u)){zero=true;}
    }
    bank.store32(dst,stored);dst+=4u;--words;
    if(zero){
      while(words--){bank.store32(dst,0);dst+=4u;}
      for(auto n=tail;n;--n)bank.store8(dst++,0);
      return;
    }
  }
  if(tail)bytes(tail);
}
void texture_camera_set_object_marker(Bank& bank,std::uint32_t object,std::uint32_t marker,std::uint32_t fov) {
  const auto scenario=bank.u32(0x4ecc20u);
  texture_camera_off(bank);
  if(!scenario)return;
  if(object==none) {
    // 256A7 clears the block and object but not the 515EDC byte.
    for(unsigned n=0;n<12;++n)bank.store32(0x4c6190u+n*4u,0);
    bank.store32(0x4c6190u,none);
    return;
  }
  const auto body=object_body(bank,object);
  const auto names=bank.u32(scenario+0x4cu);
  const auto name=std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0xacu))));
  original_strncpy(bank,0x4c6194u,names+name*36u,0x20u);
  bank.store8(0x4c61b3u,0);
  bank.store32(0x4c61b4u,marker);
  bank.store32(0x4c6190u,object);
  const auto mx=bank.floating_point().mxcsr;
  std::uint32_t stored;
  if(sse_compared(fov,mx)==0.f)stored=0;
  else if(10.f>sse_compared(fov,mx))stored=0x41200000u;
  else if(!(sse_compared(fov,mx)>160.f))stored=fov;
  else stored=0x43200000u;
  bank.store32(0x4c61b8u,stored);
}

namespace {
std::uint32_t tag_data(const Bank& bank,std::uint32_t datum) {
  return bank.u32(bank.u32(0x4f0414u)+(datum&0xffffu)*16u+8u);
}
// 144910: quaternion at `source` to the 3x3 rotation at `target`.
void quaternion_matrix(Bank& bank,std::uint32_t source,std::uint32_t target) {
  const auto mx=bank.floating_point().mxcsr;
  const auto q=[&](unsigned n){return as_float(bank.u32(source+n*4u));};
  const auto m=[mx](float a,float b){return mulss(a,b,mx);};
  const auto a=[mx](float x,float y){return addss(x,y,mx);};
  const auto s_=[mx](float x,float y){return subss(x,y,mx);};
  const auto x=q(0),y=q(1),z=q(2),w=q(3);
  const auto norm=a(a(a(m(x,x),m(y,y)),m(z,z)),m(w,w));
  const auto scale=sse_compared(as_bits(norm),mx)>sse_compared(0x38d1b717u,mx)?divss(2.f,norm,mx):0.f;
  const auto xs=m(x,scale),ys=m(y,scale),zs=m(scale,z);
  const auto wz=m(zs,w),wx=m(xs,w),xx=m(x,xs),xy=m(x,ys),xz=m(x,zs),yy=m(y,ys),wy=m(ys,w),yz=m(y,zs),zz=m(zs,z);
  bank.store32(target,as_bits(s_(1.f,a(zz,yy))));
  bank.store32(target+0xcu,as_bits(s_(xy,wz)));
  bank.store32(target+4u,as_bits(a(xy,wz)));
  bank.store32(target+0x18u,as_bits(a(xz,wy)));
  bank.store32(target+0x10u,as_bits(s_(1.f,a(zz,xx))));
  bank.store32(target+0x1cu,as_bits(s_(yz,wx)));
  bank.store32(target+8u,as_bits(s_(xz,wy)));
  bank.store32(target+0x14u,as_bits(a(yz,wx)));
  bank.store32(target+0x20u,as_bits(s_(1.f,a(yy,xx))));
}
// 145560: scaled 4x3 product A * B over 0x34-byte matrices (scale,
// forward, left, up, translation), with its exact row association. Every B
// element is read before the store that could alias it, so the original's
// in-place use (result == B) is preserved.
using Matrix=std::array<std::uint32_t,13>;
Matrix matrix_product_values(const Matrix& a,const Matrix& b,std::uint32_t mx) {
  const auto A=[&](unsigned n){return as_float(a[n]);};
  const auto B=[&](unsigned n){return as_float(b[n]);};
  const auto m=[mx](float x,float y){return mulss(x,y,mx);};
  const auto ad=[mx](float x,float y){return addss(x,y,mx);};
  const float scale=A(0);
  Matrix result{};
  for(unsigned k=0;k<3;++k)
    result[10+k]=as_bits(ad(m(ad(ad(m(B(10),A(1+k)),m(B(11),A(4+k))),m(B(12),A(7+k))),scale),A(10+k)));
  for(const unsigned first:{7u,4u,1u})
    for(unsigned k=0;k<3;++k)
      result[first+k]=as_bits(ad(m(B(first),A(1+k)),ad(m(B(first+1),A(4+k)),m(B(first+2),A(7+k)))));
  result[0]=as_bits(m(B(0),scale));
  return result;
}
Matrix load_matrix(const Bank& bank,std::uint32_t at) {
  Matrix value;for(unsigned n=0;n<13;++n)value[n]=bank.u32(at+n*4u);return value;
}
void matrix_product(Bank& bank,std::uint32_t a,std::uint32_t b,std::uint32_t result) {
  const auto r=matrix_product_values(load_matrix(bank,a),load_matrix(bank,b),bank.floating_point().mxcsr);
  bank.store32(result+0x28u,r[10]);bank.store64(result+0x2cu,r[11],r[12]);
  bank.store32(result+0x1cu,r[7]);bank.store64(result+0x20u,r[8],r[9]);
  bank.store32(result+0x10u,r[4]);bank.store64(result+0x14u,r[5],r[6]);
  bank.store32(result+4u,r[1]);bank.store64(result+8u,r[2],r[3]);
  bank.store32(result,r[0]);
}
// 144BF0: frame from forward, up and position; left = cross(up, forward).
Matrix basis_frame(const Bank& bank,std::uint32_t forward,std::uint32_t up,std::uint32_t position) {
  const auto mx=bank.floating_point().mxcsr;
  const auto f=[&](unsigned n){return as_float(bank.u32(forward+n*4u));};
  const auto u=[&](unsigned n){return as_float(bank.u32(up+n*4u));};
  const auto m=[mx](float x,float y){return mulss(x,y,mx);};
  Matrix frame{};
  frame[0]=0x3f800000u;
  for(unsigned n=0;n<3;++n){frame[1+n]=bank.u32(forward+n*4u);frame[7+n]=bank.u32(up+n*4u);frame[10+n]=bank.u32(position+n*4u);}
  frame[6]=as_bits(subss(m(f(1),u(0)),m(f(0),u(1)),mx));
  frame[5]=as_bits(subss(m(u(2),f(0)),m(f(2),u(0)),mx));
  frame[4]=as_bits(subss(m(f(2),u(1)),m(u(2),f(1)),mx));
  return frame;
}
std::uint32_t marker_group(const Bank& bank,std::uint32_t render,std::uint32_t name) {
  if(render==none || name==none || !name)return none;
  const auto model=tag_data(bank,render);
  const auto count=std::int32_t(bank.u32(model+0x58u));
  const auto groups=bank.u32(model+0x5cu);
  for(std::int32_t n=0;n<count;++n)if(bank.u32(groups+std::uint32_t(n)*12u)==name)return std::uint32_t(n);
  return none;
}
std::uint16_t model_markers(Bank& bank,std::uint32_t group,std::uint32_t render,std::uint32_t permutations,
    std::uint32_t nodes,bool mirrored,std::uint32_t output,std::int16_t count) {
  if(group==none)return 0;
  const auto row=bank.u32(tag_data(bank,render)+0x5cu)+group*12u;
  if(std::int32_t(bank.u32(row+4u))<=0)return 0;
  std::int32_t found=0;
  const auto mx=bank.floating_point().mxcsr;
  for(std::int32_t index=0;;) {
    const auto marker=bank.u32(row+8u)+std::uint32_t(index)*0x24u;
    bool admitted=true;
    if(permutations) {
      const auto region=bank.u8(marker);
      if(region!=0xffu)
        admitted=bank.u8(permutations+std::uint32_t(std::int32_t(std::int8_t(region))))==bank.u8(marker+1u);
    }
    if(admitted) {
      if(found>=std::int32_t(count))break;
      const auto record=output+std::uint32_t(found)*0x70u;++found;
      bank.store16(record,bank.u8(marker+2u));
      quaternion_matrix(bank,marker+0x10u,record+8u);
      bank.store32(record+0x2cu,0);bank.store32(record+0x30u,0);bank.store32(record+0x34u,0);
      bank.store32(record+4u,0x3f800000u);
      bank.store32(record+0x2cu,bank.u32(marker+4u));
      bank.store32(record+0x30u,bank.u32(marker+8u));
      bank.store32(record+0x34u,bank.u32(marker+0xcu));
      const auto node=std::uint32_t(std::int32_t(std::int16_t(bank.u16(record))));
      matrix_product(bank,nodes+node*0x34u,record+4u,record+0x38u);
      bank.store32(record+0x6cu,bank.u32(marker+0x20u));
      if(mirrored)for(const auto offset:{0x48u,0x4cu,0x50u})
        bank.store32(record+offset,as_bits(subss(0.f,as_float(bank.u32(record+offset)),mx)));
    }
    ++index;
    if(index>=std::int32_t(bank.u32(row+4u)))break;
  }
  return std::uint16_t(found);
}
}
std::uint32_t object_marker_owner(const Bank& bank,std::uint32_t object) {
  for(;;) {
    const auto body=object_body(bank,object);
    const auto parent=bank.u32(body+0x14u);
    if(parent!=none && ((bank.u8(tag_data(bank,bank.u32(body))+2u)>>4u)&1u)){object=parent;continue;}
    if((bank.u8(body+4u)&1u) && ((1u<<(bank.u8(body+0xaau)&31u))&0x1cu)) {
      const auto device=bank.u8(object_body(bank,object)+0x12cu);
      if((device&1u) && !((device>>1u)&1u))return bank.u32(body+0x154u);
    }
    return object;
  }
}
std::uint16_t object_get_markers(Bank& bank,std::uint32_t object,std::uint32_t marker,
    std::int16_t count,bool owner_resolved,std::uint32_t output) {
  const auto mx=bank.floating_point().mxcsr;
  const auto owner=owner_resolved?object:object_marker_owner(bank,object);
  if(owner!=none) {
    const auto body=object_body(bank,owner);
    const auto model=bank.u32(tag_data(bank,bank.u32(body))+0x38u);
    if(model!=none) {
      const auto permutations=body+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x11au))));
      const auto nodes=body+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x116u))));
      const bool mirrored=(bank.u32(body+4u)>>10u)&1u;
      const auto render=bank.u32(tag_data(bank,model)+4u);
      const auto group=marker_group(bank,render,marker);
      const auto found=model_markers(bank,group,render,permutations,nodes,mirrored,output,count);
      if(found)return found;
    }
  }
  const auto body=object_body(bank,object);
  bank.store16(output,0);
  constexpr std::array<std::uint32_t,13> identity{0x3f800000u,0x3f800000u,0,0,0,0x3f800000u,0,0,0,0x3f800000u,0,0,0};
  for(unsigned n=0;n<13;++n)bank.store32(output+4u+n*4u,identity[n]);
  const auto nodes=body+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x116u))));
  for(unsigned n=0;n<13;++n)bank.store32(output+0x38u+n*4u,bank.u32(nodes+n*4u));
  bank.store32(output+0x6cu,0);
  if((bank.u32(body+4u)>>10u)&1u)for(const auto offset:{0x48u,0x4cu,0x50u})
    bank.store32(output+offset,as_bits(subss(0.f,as_float(bank.u32(output+offset)),mx)));
  return marker?0:1;
}

namespace {
// 111CD0: the unit's current animation mode name, or NONE.
std::uint32_t unit_animation_mode(const Bank& bank,std::uint32_t unit) {
  const auto body=object_body(bank,unit);
  const auto animation=body+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x12au))));
  if(bank.u32(animation+0x68u)==none || bank.u32(animation)==none || bank.u16(animation+6u)==0xffffu)return none;
  return bank.u32(animation+0x7cu);
}
// C9550: whether that mode is one of the five 473990 entries.
bool unit_mode_listed(const Bank& bank,std::uint32_t unit) {
  const auto body=object_body(bank,unit);
  const auto mode=bank.u32(body+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x12au))))+0x7cu);
  for(std::uint32_t at=0x473990u;at<0x4739a4u;at+=4u)if(bank.u32(at)==mode)return true;
  return false;
}
// 158340: seat camera class written to `kind`; returns the SHORT result.
std::uint16_t unit_seat_camera(const Bank& bank,std::uint32_t unit,std::uint16_t& kind) {
  std::uint16_t result=0;
  const auto body=object_try_and_get_and_verify_type(bank,unit,3u);
  kind=0;
  if(!body)return 0;
  const auto parent=bank.u32(body+0x14u);
  if(parent==none)return 0;
  const auto vehicle=object_body(bank,parent);
  const auto mode=unit_animation_mode(bank,unit);
  if(((1u<<(bank.u8(vehicle+0xaau)&31u))&3u)) {
    const auto seats=bank.u32(tag_data(bank,bank.u32(vehicle))+0x1ccu);
    const auto seat=bank.u32(seats+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x1fcu))))*0xb0u);
    if((seat>>4u)&1u)result=1;
    if((seat>>6u)&1u) {
      if(unit_mode_listed(bank,unit)){kind=1;return 1;}
      if(mode==0x400004au){kind=3;return 1;}
    }
  }
  kind=2;
  return result;
}
}
void camera_control(Bank& bank,std::uint8_t active) {
  const auto users=bank.u32(0x4f54f0u);
  bank.store8(bank.u32(0x4f5504u),active);
  for(std::uint32_t user=0;user<4;++user) {
    const auto block=0x4f5514u+user*0x140u;
    if(active) {
      bank.store32(block+0xcu,0x16f6a0u);
      bank.store32(block+0x10cu,0x3f800000u);
      bank.store8(block+0x108u,0);
    } else {
      const auto player=bank.u32(users+0xcu+user*4u);
      const auto unit=player==none?none:
          bank.u32(bank.u32(bank.u32(0x4f54f4u)+0x44u)+(player&0xffffu)*0x21cu+0x2cu);
      std::uint16_t kind=0;
      const auto result=unit_seat_camera(bank,unit,kind);
      std::uint32_t mode;
      if(result==1) {
        bank.store8(block+0x10u,0);bank.store8(block+0x11u,0);bank.store8(block+0x12u,0);bank.store8(block+0x13u,0);
        bank.store16(block+0x16u,0);bank.store32(block+0x18u,none);bank.store16(block+0x1cu,0xffffu);
        bank.store32(block+0x24u,0);bank.store32(block+0x20u,0);bank.store32(block+0x28u,0x3f800000u);
        bank.store8(block+0x14u,0);
        mode=0x245ea0u;
      } else {
        bank.store32(block+0x10u,0);bank.store32(block+0x14u,0);
        mode=0x245400u;
      }
      // 158900 with its flag clear: mode, blend 1.0 and its reset byte.
      bank.store32(block+0xcu,mode);
      bank.store32(block+0x10cu,0x3f800000u);
      bank.store8(block+0x108u,0);
      bank.store16(block+0x58u,kind);
    }
    const auto director=bank.u32(0x51df6cu);
    bank.store8(director,active);
    bank.store8(director+1u,1);
  }
}

namespace {
// 1708F0: model node index by name, or NONE.
std::uint32_t model_node(const Bank& bank,std::uint32_t model,std::uint32_t name) {
  if(model==none)return none;
  const auto data=tag_data(bank,model);
  const auto count=std::int32_t(bank.u32(data+0x78u));
  const auto nodes=bank.u32(data+0x7cu);
  for(std::int32_t n=0;n<count;++n)if(bank.u32(nodes+std::uint32_t(n)*0x5cu)==name)return std::uint32_t(n);
  return none;
}
// 11EAB0: the trigger volume's frame, attached volumes composed in place with
// their object's named node.
std::optional<Matrix> trigger_volume_frame(const Bank& bank,std::int16_t volume) {
  const auto row=bank.u32(bank.u32(0x4ecc20u)+0x10cu)+std::uint32_t(std::int32_t(volume))*0x44u;
  const auto name=std::int16_t(bank.u16(row+4u));
  if(name==-1)return basis_frame(bank,row+0xcu,row+0x18u,row+0x24u);
  if(name<0 || name>=0x280)return std::nullopt;
  const auto object=bank.u32(bank.u32(0x4eaba0u)+std::uint32_t(name)*4u);
  if(object==none)return std::nullopt;
  const auto body=object_body(bank,object);
  const auto node=model_node(bank,bank.u32(tag_data(bank,bank.u32(body))+0x38u),bank.u32(row+8u));
  if(node==none)return std::nullopt;
  const auto local=basis_frame(bank,row+0xcu,row+0x18u,row+0x24u);
  const auto nodes=body+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x116u))));
  const auto index=std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(node))));
  return matrix_product_values(load_matrix(bank,nodes+index*0x34u),local,bank.floating_point().mxcsr);
}
}
bool trigger_volume_test_point(const Bank& bank,std::int16_t volume,std::uint32_t point) {
  const auto frame=trigger_volume_frame(bank,volume);
  if(!frame)return false;
  const auto mx=bank.floating_point().mxcsr;
  const auto c=[mx](std::uint32_t bits){return sse_compared(bits,mx);};
  const auto row=bank.u32(bank.u32(0x4ecc20u)+0x10cu)+std::uint32_t(std::int32_t(volume))*0x44u;
  const auto F=[&](unsigned n){return as_float((*frame)[n]);};
  const auto scale=(*frame)[0];
  if(c(scale)==0.f)return false;
  float dx=subss(as_float(bank.u32(point)),F(10),mx);
  float dy=subss(as_float(bank.u32(point+4u)),F(11),mx);
  float dz=subss(as_float(bank.u32(point+8u)),F(12),mx);
  if(!(c(scale)==1.f)) {
    const auto inverse=divss(1.f,F(0),mx);
    dx=mulss(inverse,dx,mx);dy=mulss(inverse,dy,mx);dz=mulss(inverse,dz,mx);
  }
  const auto m=[mx](float x,float y){return mulss(x,y,mx);};
  const auto a=addss(addss(m(F(3),dz),m(F(2),dy),mx),m(F(1),dx),mx);
  const auto b=addss(addss(m(F(6),dz),m(F(5),dy),mx),m(F(4),dx),mx);
  const auto cc=addss(addss(m(F(9),dz),m(F(8),dy),mx),m(F(7),dx),mx);
  return c(as_bits(a))>0.f && c(as_bits(b))>0.f && c(as_bits(cc))>0.f &&
      c(bank.u32(row+0x30u))>c(as_bits(a)) && c(bank.u32(row+0x34u))>c(as_bits(b)) && c(bank.u32(row+0x38u))>c(as_bits(cc));
}
bool volume_test_object(const Bank& bank,std::int16_t volume,std::uint32_t object) {
  return object!=none && trigger_volume_test_point(bank,volume,object_body(bank,object)+0x30u);
}
bool volume_test_objects(const Bank& bank,std::int16_t volume,std::span<const std::uint32_t> members,bool all) {
  for(const auto member:members) {
    if(member==none)break;
    const bool inside=trigger_volume_test_point(bank,volume,object_body(bank,member)+0x30u);
    if(inside && !all)return true;
    if(!inside && all)return false;
  }
  return all;
}

std::uint32_t bsp_leaf(const Bank& bank,std::uint32_t bsp,std::array<std::uint32_t,3> point) {
  const auto mx=bank.floating_point().mxcsr;
  const auto nodes=bank.u32(bsp+4u),planes=bank.u32(bsp+0xcu);
  std::uint32_t index=0;
  for(;;) {
    const auto node=nodes+(index&0x7fffffu)*8u;
    const auto plane=planes+std::uint32_t(std::int32_t(std::int16_t(bank.u16(node))))*16u;
    const auto P=[&](unsigned n){return as_float(bank.u32(plane+n*4u));};
    const auto distance=subss(addss(addss(mulss(P(2),as_float(point[2]),mx),mulss(P(1),as_float(point[1]),mx),mx),
        mulss(P(0),as_float(point[0]),mx),mx),P(3),mx);
    // COMISS d,0; JB selects child 0 (below or unordered).
    const auto value=sse_compared(as_bits(distance),mx);
    const unsigned side=(value>=0.f)?1u:0u;
    // Children are 24-bit signed fields at node+2 and node+5.
    const auto child=std::int32_t(bank.u32(node+3u*side+1u))>>8;
    if(!(child&0x800000)){index=std::uint32_t(child);continue;}
    return child==-1?none:std::uint32_t(child)&0x7fffffu;
  }
}
NativeCampaignLocation scenario_location(const Bank& bank,std::array<std::uint32_t,3> point) {
  const auto bsp=bank.u16(0x474c24u);
  if(bsp==0xffffu)return {none,0xffffu,0xffffu};
  const auto leaf=bsp_leaf(bank,bank.u32(0x4ecc0cu),point);
  if(leaf==none)return {none,0xffffu,bsp};
  const auto cluster=bank.u16(bank.u32(bank.u32(0x4ecc18u)+0x30u)+leaf*8u);
  return {leaf,cluster,bsp};
}
namespace {
// 123210: dominant axis of a normal (x87 FABS/FCOMI then COMISS).
unsigned dominant_axis(const Bank& bank,std::uint32_t normal) {
  const auto mx=bank.floating_point().mxcsr;
  const auto x=as_bits(std::fabs(as_float(bank.u32(normal)))),y=as_bits(std::fabs(as_float(bank.u32(normal+4u)))),
      z=as_bits(std::fabs(as_float(bank.u32(normal+8u))));
  const bool z_below_y=!(as_float(z)>=as_float(y)); // FCOMI: below or unordered
  if(!z_below_y && !(sse_compared(z,mx)<sse_compared(x,mx) || std::isnan(sse_compared(z,mx)) || std::isnan(sse_compared(x,mx))))return 2;
  if(!(sse_compared(y,mx)<sse_compared(x,mx) || std::isnan(sse_compared(y,mx)) || std::isnan(sse_compared(x,mx))))return 1;
  return 0;
}
// 243450: projected point against a convex portal polygon widened by radius.
bool portal_polygon(const Bank& bank,std::span<const std::array<float,2>> vertices,std::int16_t count,
    std::array<float,2> point,float radius) {
  const auto mx=bank.floating_point().mxcsr;
  const auto squared=mulss(radius,radius,mx);
  for(std::int16_t i=0;i<count;++i) {
    const auto j=i+1>=count?0:i+1;
    const auto& a=vertices[std::size_t(i)];const auto& b=vertices[std::size_t(j)];
    const auto dx=subss(point[0],a[0],mx),dy=subss(point[1],a[1],mx);
    const auto ex=subss(b[0],a[0],mx),ey=subss(b[1],a[1],mx);
    const auto length=addss(mulss(ex,ex,mx),mulss(ey,ey,mx),mx);
    if(sse_compared(as_bits(length),mx)==0.f)continue;
    const auto cross=subss(mulss(dx,ey,mx),mulss(dy,ex,mx),mx);
    if(!(sse_compared(as_bits(cross),mx)>0.f))continue;
    if(sse_compared(as_bits(mulss(cross,cross,mx)),mx)>sse_compared(as_bits(mulss(length,squared,mx)),mx))return false;
  }
  return true;
}
#if HALO2_NATIVE_SCRIPT_X86_FP
// 14CEC8..14CEF0: plane distance at x87 precision, its FST dword copy, and
// FCOMI radius > |distance| on the unrounded value.
std::pair<float,bool> portal_plane_distance(const std::array<float,4>& p,const std::array<float,3>& c,float radius,
    std::uint16_t control) {
  const auto mode=x87_control(control);std::uint16_t saved{};float stored{};std::uint8_t inside{};
  asm volatile("fnstcw %2\n\tfldcw %3\n\t"
      "flds %4\n\tfmuls %5\n\tflds %6\n\tfmuls %7\n\tfaddp\n\tflds %8\n\tfmuls %9\n\tfaddp\n\tfsubs %10\n\t"
      "fsts %0\n\tfabs\n\tflds %11\n\tfcomip %%st(1), %%st\n\tseta %1\n\tfstp %%st(0)\n\tfldcw %2"
      :"=m"(stored),"=q"(inside),"+m"(saved)
      :"m"(mode),"m"(p[1]),"m"(c[1]),"m"(p[2]),"m"(c[2]),"m"(c[0]),"m"(p[0]),"m"(p[3]),"m"(radius)
      :"st","st(1)","st(2)","cc");
  return {stored,inside!=0};
}
// 14D026..14D04F: FSQRT(radius*radius - d*d) stored as a dword.
float portal_radius(float radius,float distance,std::uint16_t control) {
  const auto mode=x87_control(control);std::uint16_t saved{};float result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %4\n\tfmuls %4\n\tflds %3\n\tfmuls %3\n\t"
      "fsub %%st(1), %%st\n\tfsqrt\n\tfstps %0\n\tfstp %%st(0)\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(radius),"m"(distance):"st","st(1)");
  return result;
}
#else
std::pair<float,bool> portal_plane_distance(const std::array<float,4>& p,const std::array<float,3>& c,float radius,
    std::uint16_t) {
  const double d=((double(p[1])*c[1])+(double(p[2])*c[2]))+(double(c[0])*p[0])-double(p[3]);
  return {float(d),double(radius)>std::fabs(d)};
}
float portal_radius(float radius,float distance,std::uint16_t) {
  return float(std::sqrt(double(radius)*radius-double(distance)*distance));
}
#endif
constexpr std::array<std::array<std::int16_t,2>,6> projection_axes{{{2,1},{1,2},{0,2},{2,0},{1,0},{0,1}}};
// 14CEA0: whether the query sphere passes through a portal.
bool sphere_through_portal(const Bank& bank,std::int16_t portal_index,std::array<std::uint32_t,3> center_bits,
    std::uint32_t structure,std::uint32_t radius_bits) {
  const auto& fp=bank.floating_point();const auto mx=fp.mxcsr;
  const auto portal=bank.u32(structure+0x60u)+std::uint32_t(std::int32_t(portal_index))*36u;
  const auto plane_index=bank.u32(portal+4u);
  const auto plane=bank.u32(bank.u32(structure+0x18u)+0xcu)+(plane_index<<4u);
  std::array<float,4> P;for(unsigned n=0;n<4;++n)P[n]=as_float(bank.u32(plane+n*4u));
  const std::array<float,3> c{as_float(center_bits[0]),as_float(center_bits[1]),as_float(center_bits[2])};
  const auto radius=as_float(radius_bits);
  const auto [distance,near_plane]=portal_plane_distance(P,c,radius,fp.x87_control_word);
  if(!near_plane)return false;
  const auto dz=subss(as_float(bank.u32(portal+0x10u)),c[2],mx);
  const auto dx=subss(as_float(bank.u32(portal+8u)),c[0],mx);
  const auto dy=subss(as_float(bank.u32(portal+0xcu)),c[1],mx);
  const auto reach=addss(as_float(bank.u32(portal+0x14u)),radius,mx);
  const auto squared=addss(addss(mulss(dz,dz,mx),mulss(dx,dx,mx),mx),mulss(dy,dy,mx),mx);
  if(!(sse_compared(as_bits(mulss(reach,reach,mx)),mx)>sse_compared(as_bits(squared),mx)))return false;
  const auto normal=bank.u32(bank.u32(0x4ecc0cu)+0xcu)+(plane_index<<4u);
  const auto axis=dominant_axis(bank,normal);
  const unsigned positive=sse_compared(bank.u32(normal+axis*4u),mx)>0.f?1u:0u;
  const auto negated=subss(0.f,distance,mx);
  std::array<float,3> projected;
  for(unsigned n=0;n<3;++n)projected[n]=addss(mulss(as_float(bank.u32(normal+n*4u)),negated,mx),c[n],mx);
  const auto [u,v]=projection_axes[positive+axis*2u];
  const auto count=std::int32_t(bank.u32(portal+0x1cu));
  const auto points=bank.u32(portal+0x20u);
  std::vector<std::array<float,2>> vertices;
  for(std::int32_t k=0;k<count;++k) {
    const auto vertex=points+std::uint32_t(k)*12u;
    vertices.push_back({as_float(bank.u32(vertex+std::uint32_t(u)*4u)),as_float(bank.u32(vertex+std::uint32_t(v)*4u))});
  }
  const auto widened=portal_radius(radius,distance,fp.x87_control_word);
  return portal_polygon(bank,vertices,std::int16_t(std::uint16_t(count)),
      {projected[std::size_t(u)],projected[std::size_t(v)]},widened);
}
// 14D0E0: recursive portal flood from `cluster`; appends visited clusters.
std::int16_t flood_clusters(Bank& bank,std::int16_t cluster,std::array<std::uint32_t,3> center,std::uint32_t radius,
    std::int32_t capacity,std::vector<std::int16_t>& output) {
  const auto structure=bank.u32(0x4ecc18u);
  const auto row=bank.u32(structure+0xa0u)+std::uint32_t(std::int32_t(cluster))*0xb0u;
  auto remaining=capacity-1;
  if(std::int16_t(std::uint16_t(capacity))>0)output.push_back(cluster);
  const auto counter=bank.u32(0x4f3ce4u);
  const auto mark=0x4f3ce8u+std::uint32_t(std::int32_t(cluster))*4u;
  if(bank.u32(mark)!=counter)bank.store32(mark,counter);
  std::int32_t count=1;
  for(std::int16_t i=0;std::int32_t(i)<std::int32_t(bank.u32(row+0x8cu));++i) {
    const auto portal_index=std::int16_t(bank.u16(bank.u32(row+0x90u)+std::uint32_t(i)*2u));
    const auto portal=bank.u32(structure+0x60u)+std::uint32_t(std::int32_t(portal_index))*36u;
    auto other=std::int16_t(bank.u16(portal));
    if(other==cluster)other=std::int16_t(bank.u16(portal+2u));
    if(bank.u32(0x4f3ce8u+std::uint32_t(std::int32_t(other))*4u)==bank.u32(0x4f3ce4u))continue;
    if(!sphere_through_portal(bank,portal_index,center,structure,radius))continue;
    const auto added=flood_clusters(bank,other,center,radius,remaining,output);
    count+=added;remaining-=added;
  }
  return std::int16_t(count);
}
}
std::vector<std::uint32_t> objects_in_sphere(Bank& bank,std::uint32_t list_flags,std::uint32_t type_mask,
    const NativeCampaignLocation& location,std::array<std::uint32_t,3> center,std::uint32_t radius,std::int16_t maximum,
    std::uint32_t buffer) {
  // `buffer` reproduces the original caller-array stores when it is caller
  // memory; NONE keeps the result private (11ED20 owns a stack array).
  const auto mx=bank.floating_point().mxcsr;
  if(!type_mask)type_mask=none;
  if(!list_flags)list_flags=none;
  std::vector<std::int16_t> clusters;
  std::int16_t count=0;
  if(location.cluster!=0xffffu) {
    if(sse_compared(radius,mx)>0.f) {
      bank.store32(0x4f3ce4u,bank.u32(0x4f3ce4u)+1u);
      bank.store8(0x4f3ce1u,1);
      count=flood_clusters(bank,std::int16_t(location.cluster),center,radius,0x200,clusters);
      bank.store8(0x4f3ce1u,0);
      if(count>0x200)count=0x200;
    } else {count=1;clusters.assign(1,std::int16_t(location.cluster));}
  }
  bank.store32(0x4eabccu,bank.u32(0x4eabccu)+1u);
  bank.store8(0x4eabc8u,1);
  std::vector<std::uint32_t> found;
  const auto scan=[&](std::uint32_t table,std::uint32_t pool,std::int16_t cluster,bool first)->bool {
    auto object=bank.u32(bank.u32(table)+std::uint32_t(std::int32_t(cluster))*4u);
    if(object==none)return true;
    const auto rows=bank.u32(pool);
    auto row=bank.u32(rows+0x44u)+(object&0xffffu)*bank.u32(rows+0x24u);
    auto next=bank.u32(row+8u);auto payload=row+0xcu;object=bank.u32(row+4u);
    for(;;) {
      if(type_mask&(1u<<(bank.u8(payload)&31u))) {
        const auto counter=bank.u32(0x4eabccu);
        const auto mark=0x4eabd0u+(object&0xffffu)*4u;
        if(bank.u32(mark)!=counter) {
          bank.store32(mark,counter);
          const auto dx=subss(as_float(bank.u32(payload+4u)),as_float(center[0]),mx);
          const auto dy=subss(as_float(bank.u32(payload+8u)),as_float(center[1]),mx);
          const auto dz=subss(as_float(bank.u32(payload+0xcu)),as_float(center[2]),mx);
          const auto reach=addss(as_float(bank.u32(payload+0x10u)),as_float(radius),mx);
          const auto squared=first?
              addss(addss(mulss(dz,dz,mx),mulss(dy,dy,mx),mx),mulss(dx,dx,mx),mx):
              addss(addss(mulss(dz,dz,mx),mulss(dx,dx,mx),mx),mulss(dy,dy,mx),mx);
          const auto limit=mulss(reach,reach,mx);
          if(!(sse_compared(as_bits(limit),mx)<sse_compared(as_bits(squared),mx) ||
              std::isnan(sse_compared(as_bits(limit),mx)) || std::isnan(sse_compared(as_bits(squared),mx)))) {
            if(std::int16_t(found.size())>=maximum)return false;
            if(buffer!=none)bank.store32(buffer+std::uint32_t(found.size())*4u,object);
            found.push_back(object);
          }
        }
      }
      if(next==none)return true;
      row=bank.u32(rows+0x44u)+(next&0xffffu)*bank.u32(rows+0x24u);
      next=bank.u32(row+8u);payload=row+0xcu;object=bank.u32(row+4u);
      if(object==none)return true;
    }
  };
  for(std::int16_t c=0;c<count;++c) {
    const auto cluster=clusters.at(std::size_t(c));
    if((list_flags&1u) && !scan(0x4eabb0u,0x4eabb4u,cluster,true)){bank.store8(0x4eabc8u,0);return found;}
    if((list_flags&2u) && !scan(0x4eaba4u,0x4eaba8u,cluster,false)){bank.store8(0x4eabc8u,0);return found;}
  }
  bank.store8(0x4eabc8u,0);
  return found;
}
std::vector<std::uint32_t> volume_return_objects(Bank& bank,std::int16_t volume,std::uint32_t type_mask) {
  const auto frame=trigger_volume_frame(bank,volume);
  if(!frame)return {};
  const auto mx=bank.floating_point().mxcsr;
  const auto row=bank.u32(bank.u32(0x4ecc20u)+0x10cu)+std::uint32_t(std::int32_t(volume))*0x44u;
  const auto F=[&](unsigned n){return as_float((*frame)[n]);};
  auto hx=mulss(as_float(bank.u32(row+0x30u)),0.5f,mx),hy=mulss(as_float(bank.u32(row+0x34u)),0.5f,mx),
      hz=mulss(as_float(bank.u32(row+0x38u)),0.5f,mx);
  if(!(sse_compared((*frame)[0],mx)==1.f)){hx=mulss(F(0),hx,mx);hy=mulss(F(0),hy,mx);hz=mulss(hz,F(0),mx);}
  const auto m=[mx](float a,float b){return mulss(a,b,mx);};
  const auto a=[mx](float x,float y){return addss(x,y,mx);};
  const std::array<std::uint32_t,3> center{
      as_bits(a(a(a(m(F(4),hy),m(F(1),hx)),m(F(7),hz)),F(10))),
      as_bits(a(a(a(m(F(8),hz),m(F(5),hy)),m(F(2),hx)),F(11))),
      as_bits(a(a(a(m(F(9),hz),m(F(6),hy)),m(F(3),hx)),F(12)))};
  const auto location=scenario_location(bank,center);
  if(location.cluster==0xffffu)return {};
  const auto candidates=objects_in_sphere(bank,0,type_mask,location,center,bank.u32(row+0x3cu),0x80);
  std::vector<std::uint32_t> members;
  for(const auto object:candidates) {
    if(object==none)continue;
    if(trigger_volume_test_point(bank,volume,object_body(bank,object)+0x30u))members.push_back(object);
  }
  return members;
}

namespace {
#if HALO2_NATIVE_SCRIPT_X86_FP
float x87_subtract(float a,float b,std::uint16_t control) {
  const auto mode=x87_control(control);std::uint16_t saved{};float result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfsubs %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(a),"m"(b):"st");
  return result;
}
// 30BD1..30C00: sqrt((x*x + y*y) + z*z), its dword copy, 1e-4 > |length|.
std::pair<float,bool> x87_length(float x,float y,float z,std::uint16_t control) {
  const auto mode=x87_control(control);std::uint16_t saved{};float stored{};std::uint8_t tiny{};
  const float threshold=std::bit_cast<float>(0x38d1b717u);
  asm volatile("fnstcw %2\n\tfldcw %3\n\t"
      "flds %4\n\tfmul %%st(0), %%st\n\tflds %5\n\tfmul %%st(0), %%st\n\tfaddp\n\tflds %6\n\tfmul %%st(0), %%st\n\tfaddp\n\t"
      "fsqrt\n\tfsts %0\n\tfabs\n\tflds %7\n\tfcomip %%st(1), %%st\n\tseta %1\n\tfstp %%st(0)\n\tfldcw %2"
      :"=m"(stored),"=q"(tiny),"+m"(saved):"m"(mode),"m"(x),"m"(y),"m"(z),"m"(threshold):"st","st(1)","cc");
  return {stored,tiny!=0};
}
// CE81B..CE850: ((dz*az + dy*ay) + dx*ax) > FCOS(angle).
bool x87_facing(const std::array<float,3>& d,const std::array<float,3>& aim,float angle,std::uint16_t control) {
  const auto mode=x87_control(control);std::uint16_t saved{};std::uint8_t facing{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\t"
      "flds %3\n\tfmuls %4\n\tflds %5\n\tfmuls %6\n\tfaddp\n\tflds %7\n\tfmuls %8\n\tfaddp\n\t"
      "flds %9\n\tfcos\n\tfxch %%st(1)\n\tfcomip %%st(1), %%st\n\tseta %0\n\tfstp %%st(0)\n\tfldcw %1"
      :"=q"(facing),"+m"(saved)
      :"m"(mode),"m"(d[2]),"m"(aim[2]),"m"(d[1]),"m"(aim[1]),"m"(d[0]),"m"(aim[0]),"m"(angle)
      :"st","st(1)","st(2)","cc");
  return facing!=0;
}
#else
float x87_subtract(float a,float b,std::uint16_t){return float(double(a)-double(b));}
std::pair<float,bool> x87_length(float x,float y,float z,std::uint16_t) {
  const double length=std::sqrt((double(x)*x+double(y)*y)+double(z)*z);
  return {float(length),double(std::bit_cast<float>(0x38d1b717u))>std::fabs(length)};
}
bool x87_facing(const std::array<float,3>& d,const std::array<float,3>& aim,float angle,std::uint16_t) {
  return ((double(d[2])*aim[2]+double(d[1])*aim[1])+double(d[0])*aim[0])>std::cos(double(angle));
}
#endif
// BAD60 head marker (0x04000095, owner resolution) WORLD point, computed in
// a private scratch record like the original caller-stack array.
std::array<std::uint32_t,3> head_point(const Bank& bank,std::uint32_t object) {
  NativeCampaignScriptScratch scratch(bank.memory(),0x70u);
  NativeCampaignScriptBank local(scratch,{},bank.floating_point());
  object_get_markers(local,object,0x04000095u,1,false,NativeCampaignScriptScratch::base);
  return {local.u32(NativeCampaignScriptScratch::base+0x60u),local.u32(NativeCampaignScriptScratch::base+0x64u),
      local.u32(NativeCampaignScriptScratch::base+0x68u)};
}
}
bool unit_can_see_point(const Bank& bank,std::uint32_t viewer,std::array<std::uint32_t,3> point,std::uint32_t angle) {
  if(viewer==none)return false;
  const auto& fp=bank.floating_point();const auto mx=fp.mxcsr;
  const auto body=object_body(bank,viewer);
  const auto eye=head_point(bank,viewer);
  std::array<float,3> d;
  for(unsigned n=0;n<3;++n)d[n]=x87_subtract(as_float(point[n]),as_float(eye[n]),fp.x87_control_word);
  const auto [length,tiny]=x87_length(d[0],d[1],d[2],fp.x87_control_word);
  if(!tiny) {
    const auto inverse=divss(1.f,length,mx);
    d[0]=mulss(d[0],inverse,mx);d[1]=mulss(inverse,d[1],mx);d[2]=mulss(inverse,d[2],mx);
  }
  const std::array<float,3> aim{as_float(bank.u32(body+0x18cu)),as_float(bank.u32(body+0x190u)),as_float(bank.u32(body+0x194u))};
  return x87_facing(d,aim,as_float(angle),fp.x87_control_word);
}
bool objects_can_see_flag(const Bank& bank,std::span<const std::uint32_t> viewers,std::int16_t flag,std::uint32_t degrees) {
  const auto mx=bank.floating_point().mxcsr;
  for(const auto viewer:viewers) {
    if(viewer==none)break;
    if(!object_try_and_get_and_verify_type(bank,viewer,3u))continue;
    if(flag<0)continue;
    const auto scenario=bank.u32(0x4ecc20u);
    if(std::int32_t(flag)>=std::int32_t(bank.u32(scenario+0x1e0u)))continue;
    const auto angle=as_bits(mulss(as_float(degrees),as_float(0x3c8efa35u),mx));
    const auto position=bank.u32(scenario+0x1e4u)+std::uint32_t(std::int32_t(flag))*0x38u+0x24u;
    if(unit_can_see_point(bank,viewer,{bank.u32(position),bank.u32(position+4u),bank.u32(position+8u)},angle))return true;
  }
  return false;
}
bool objects_can_see_object(const Bank& bank,std::span<const std::uint32_t> viewers,std::uint32_t target,std::uint32_t degrees) {
  const auto mx=bank.floating_point().mxcsr;
  for(const auto viewer:viewers) {
    if(viewer==none)break;
    if(!object_try_and_get_and_verify_type(bank,viewer,3u))continue;
    if(target==none)continue;
    std::array<std::uint32_t,3> point;
    if(object_try_and_get_and_verify_type(bank,target,3u))point=head_point(bank,target);
    else {
      const auto at=object_body(bank,target)+0x30u;
      point={bank.u32(at),bank.u32(at+4u),bank.u32(at+8u)};
    }
    const auto angle=as_bits(mulss(as_float(degrees),as_float(0x3c8efa35u),mx));
    if(unit_can_see_point(bank,viewer,point,angle))return true;
  }
  return false;
}
}
