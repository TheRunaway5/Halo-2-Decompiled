// Native model-state owners of TU1.5 object_new (track "modelstate"): change
// colors (C0270 with 37DC0A/1343E0/134510/3DD00), function colors (C0910) and
// regions/permutations (BC5C0 with BC6C0/1700E0/17E060/1CE0E0). Each owner
// replicates its original routine; the oracle cases are in
// tools/native_object_factory_cases_modelstate.py and the harness kernels in
// tests/native_object_factory_modelstate_kernels.cpp. Stack-only stores of the
// originals (locals, candidate arrays) have no native counterpart.
#include "halo2/native_object_factory_modelstate_tu15.hpp"
#include "halo2/native_object_factory_attachments_tu15.hpp"
#include "halo2/native_object_factory_havok_tu15.hpp"
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <array>
#include <cmath>
#include <optional>
#include <vector>

namespace halo2::native_object_factory_tu15 {
namespace {
using namespace detail;
namespace fp=native_campaign_script_fp;
using Rgb=std::array<std::uint32_t,3>;

// .rdata constants (immutable original image bytes).
constexpr std::uint32_t one_bits=0x3f800000u;      // [46A0D0] 1.0
constexpr std::uint32_t half_bits=0x3f000000u;     // [46A0CC] 0.5
constexpr std::uint32_t two_bits=0x40000000u;      // [46A11C] 2.0
constexpr std::uint32_t four_bits=0x40800000u;     // [46A120] 4.0
constexpr std::uint32_t six_bits=0x40c00000u;      // [46A10C] 6.0
constexpr std::uint32_t sixth_bits=0x3e2aaaabu;    // [46A1B8] 1/6
constexpr std::uint32_t byte_scale_bits=0x437f0000u; // [46A148] 255.0
constexpr std::uint32_t random_scale_bits=0x37800080u; // [46A160] 1.5259022e-5
// C0270's position hash weights [46A470..46A47C] and [46A46C].
constexpr std::uint32_t hash_y_bits=0x4412c849u,hash_index_bits=0x43d79081u,hash_x_bits=0x439df252u,
  hash_z_bits=0x443a07f2u,hash_secondary_bits=0x3f364cd7u;
constexpr double fmod_divisor=1.0;                 // [46A1D0]
constexpr std::uint32_t default_color_pointer=0x474c70u; // .data: pointer to the default RGB
constexpr std::uint32_t random_seed_pointer=0x4f3cd8u;
constexpr std::uint32_t render_rows=0x515f14u;
constexpr std::uint32_t effect_pool=0x4f720cu;
constexpr std::uint32_t havok_components=0x52be5cu;

float f(std::uint32_t value){return f32(value);}
bool greater(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return fp::sse_compared(a,mxcsr)>fp::sse_compared(b,mxcsr);}
bool greater_equal(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return fp::sse_compared(a,mxcsr)>=fp::sse_compared(b,mxcsr);}
// UCOMISS ... LAHF/TEST AH,44h/JP falls through only for ordered equality.
bool equal(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return fp::sse_compared(a,mxcsr)==fp::sse_compared(b,mxcsr);}
std::uint32_t mul(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(fp::mulss(f(a),f(b),mxcsr));}
std::uint32_t add(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(fp::addss(f(a),f(b),mxcsr));}
std::uint32_t sub(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(fp::subss(f(a),f(b),mxcsr));}
std::uint32_t div(std::uint32_t a,std::uint32_t b,std::uint32_t mxcsr){return bits(fp::divss(f(a),f(b),mxcsr));}
std::uint16_t x87_mode(std::uint16_t control){return std::uint16_t(control|0x3fu);}

#if HALO2_NATIVE_SCRIPT_X86_FP
std::int32_t cvttss2si(std::uint32_t value,std::uint32_t mxcsr) {
  const auto mode=fp::sse_control(mxcsr);std::uint32_t saved{};std::int32_t result{};const float input=f(value);
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\tcvttss2si %3, %0\n\tldmxcsr %1":"=r"(result),"+m"(saved):"m"(mode),"x"(input));
  return result;
}
// C0378..C03C6: FLD z, FMUL, FLD x, FMUL, FADDP, FLD index, FMUL, FADDP,
// FLD y, FMUL, FADDP, FABS (the extended value FST/37DC0A receive).
long double position_hash(float x,float y,float z,float index,std::uint16_t control) {
  const auto mode=x87_mode(control);std::uint16_t saved{};long double result{};
  const float kz=f(hash_z_bits),kx=f(hash_x_bits),ki=f(hash_index_bits),ky=f(hash_y_bits);
  asm volatile("fnstcw %1\n\tfldcw %2\n\t"
      "flds %3\n\tfmuls %4\n\tflds %5\n\tfmuls %6\n\tfaddp\n\t"
      "flds %7\n\tfmuls %8\n\tfaddp\n\tflds %9\n\tfmuls %10\n\tfaddp\n\tfabs\n\t"
      "fstpt %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(z),"m"(kz),"m"(x),"m"(kx),"m"(index),"m"(ki),"m"(y),"m"(ky)
      :"st","st(1)");
  return result;
}
// C03D9..C03E6: FLD index, FMUL [46A46C], FADD y, FABS.
long double secondary_hash(float y,float index,std::uint16_t control) {
  const auto mode=x87_mode(control);std::uint16_t saved{};long double result{};const float k=f(hash_secondary_bits);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfmuls %4\n\tfadds %5\n\tfabs\n\tfstpt %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(index),"m"(k),"m"(y):"st");
  return result;
}
// 37DC14: FXCH, FPREM until C2 clears, FSTP ST(1). FPREM is exact, so the
// 133F working control 331ECB loads does not change the remainder.
long double partial_remainder(long double value,double divisor) {
  long double result{};
  asm volatile("fldl %2\n\tfldt %1\n\t"
      "1:\n\tfprem\n\tfnstsw %%ax\n\ttestw $0x400, %%ax\n\tjnz 1b\n\tfstp %%st(1)\n\tfstpt %0"
      :"=m"(result):"m"(value),"m"(divisor):"ax","cc","st","st(1)");
  return result;
}
// FSTP dword of an extended value under the caller's rounding control.
std::uint32_t store_single(long double value,std::uint16_t control) {
  const auto mode=x87_mode(control);std::uint16_t saved{};float result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfldt %3\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(value):"st");
  return bits(result);
}
// C0517..C0539: FLD h1, FSUB h2, FABS, FLD 0.5, FXCH, FCOMIP, FSTP; JBE.
bool hue_gap_above_half(std::uint32_t first,std::uint32_t second,std::uint16_t control) {
  const auto mode=x87_mode(control);std::uint16_t saved{};unsigned char above{};
  const float a=f(first),b=f(second),half=f(half_bits);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfsubs %4\n\tfabs\n\tflds %5\n\tfxch %%st(1)\n\t"
      "fcomip %%st(1), %%st\n\tfstp %%st(0)\n\tseta %0\n\tfldcw %1"
      :"=q"(above),"+m"(saved):"m"(mode),"m"(a),"m"(b),"m"(half):"cc","st","st(1)");
  return above!=0;
}
// 1709CB/1709D7/1709E4: FILD draw, FMUL [46A160], FSTP dword.
std::uint32_t random_unit(std::uint32_t draw,std::uint16_t control) {
  const auto mode=x87_mode(control);std::uint16_t saved{};float result{};
  const auto integer=std::int32_t(draw);const float scale=f(random_scale_bits);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(integer),"m"(scale):"st");
  return bits(result);
}
// 170BF6..170C10: FILD draw, FMUL [46A160], FMUL total, FSTP dword.
std::uint32_t random_scaled(std::uint32_t draw,std::uint32_t total,std::uint16_t control) {
  const auto mode=x87_mode(control);std::uint16_t saved{};float result{};
  const auto integer=std::int32_t(draw);const float scale=f(random_scale_bits),sum=f(total);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tfmuls %5\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(integer),"m"(scale),"m"(sum):"st");
  return bits(result);
}
// 1349E0: FLD channel, FMUL 255 (one product per channel), FISTP dword.
std::uint32_t byte_channel(std::uint32_t value,std::uint16_t control) {
  const auto mode=x87_mode(control);std::uint16_t saved{};std::int32_t result{};
  const float channel=f(value),scale=f(byte_scale_bits);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfmuls %4\n\tfistpl %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(channel),"m"(scale):"st");
  return std::uint32_t(result)&0xffu;
}
#else
[[noreturn]] void x87_unavailable(){throw std::runtime_error("Native object factory modelstate needs x86 floating point");}
std::int32_t cvttss2si(std::uint32_t,std::uint32_t){x87_unavailable();}
long double position_hash(float,float,float,float,std::uint16_t){x87_unavailable();}
long double secondary_hash(float,float,std::uint16_t){x87_unavailable();}
long double partial_remainder(long double,double){x87_unavailable();}
std::uint32_t store_single(long double,std::uint16_t){x87_unavailable();}
bool hue_gap_above_half(std::uint32_t,std::uint32_t,std::uint16_t){x87_unavailable();}
std::uint32_t random_unit(std::uint32_t,std::uint16_t){x87_unavailable();}
std::uint32_t random_scaled(std::uint32_t,std::uint32_t,std::uint16_t){x87_unavailable();}
std::uint32_t byte_channel(std::uint32_t,std::uint16_t){x87_unavailable();}
#endif

// 37DC0A (CRT fmod by the 1.0 double): 331ECB classifies both operands with
// FXAM through the [46ECB4] table and jumps through [48C8DC + selector]. The
// divisor is normal; a normal dividend selects 37DC14 (FPREM), a zero or
// denormal one 331F68 (FLDZ). Infinite/NaN dividends reach the CRT error
// methods (3334F0), which are not native. [46ECD0] is nonzero and [48D4F4]
// zero in TU1.5, and the precision exception stays masked, so 37D46C/37D536
// only save and restore the control word.
long double fmod_by_one(long double value) {
  switch(std::fpclassify(value)) {
  case FP_NORMAL:return partial_remainder(value,fmod_divisor);
  case FP_ZERO:case FP_SUBNORMAL:return 0.0L;
  default:refuse("3334F0","CRT fmod of an infinite or NaN position hash (37DC0A error method)");
  }
}

// 1343E0 with its stores reported through `store(offset, bits)`.
struct Hsv {std::uint32_t hue,saturation,value;};
template<class Store>Hsv rgb_to_hsv(const Rgb& rgb,std::uint32_t mxcsr,Store&& store) {
  const auto r=rgb[0],g=rgb[1],b=rgb[2];
  std::uint32_t maximum;
  if(greater(r,greater(g,b,mxcsr)?g:b,mxcsr))maximum=r;
  else maximum=greater(g,b,mxcsr)?g:b;
  const auto smaller_gb=greater(g,b,mxcsr)?b:g;
  std::uint32_t minimum;
  if(!greater(r,smaller_gb,mxcsr))minimum=r;
  else minimum=greater(g,b,mxcsr)?b:g;
  const auto delta=sub(maximum,minimum,mxcsr);
  store(8u,maximum);
  const auto saturation=equal(maximum,0,mxcsr)?0u:div(delta,maximum,mxcsr);
  store(4u,saturation);
  if(equal(saturation,0,mxcsr)){store(0u,0u);return {0u,saturation,maximum};}
  std::uint32_t hue;
  if(equal(r,maximum,mxcsr))hue=div(sub(g,b,mxcsr),delta,mxcsr);
  else if(equal(g,maximum,mxcsr))hue=add(div(sub(b,r,mxcsr),delta,mxcsr),two_bits,mxcsr);
  else hue=add(div(sub(r,g,mxcsr),delta,mxcsr),four_bits,mxcsr);
  store(0u,hue);
  hue=mul(hue,sixth_bits,mxcsr);
  store(0u,hue);
  if(greater(0,hue,mxcsr)){hue=add(hue,one_bits,mxcsr);store(0u,hue);}
  return {hue,saturation,maximum};
}
// 134510: `value()` re-reads the value word where the original does.
template<class Value>void hsv_to_rgb(M& m,std::uint32_t out,std::uint32_t hue,std::uint32_t saturation,
    Value&& value,std::uint32_t mxcsr) {
  const auto scaled=mul(hue,six_bits,mxcsr);
  if(equal(saturation,0,mxcsr)) {
    const auto v=value();
    w32(m,out+8u,v);w32(m,out+4u,v);w32(m,out,v);
    return;
  }
  auto sector=cvttss2si(scaled,mxcsr);
  if(greater(bits(fp::cvtsi2ss(sector,mxcsr)),scaled,mxcsr))--sector;
  const auto fraction=sub(scaled,bits(fp::cvtsi2ss(sector,mxcsr)),mxcsr);
  const auto q=mul(sub(one_bits,mul(saturation,fraction,mxcsr),mxcsr),value(),mxcsr);
  const auto p=mul(sub(one_bits,saturation,mxcsr),value(),mxcsr);
  const auto t=mul(sub(one_bits,mul(sub(one_bits,fraction,mxcsr),saturation,mxcsr),mxcsr),value(),mxcsr);
  // JMP [EAX*4+134610h]: the scaled index wraps, so a CVTTSS2SI indefinite
  // (NaN or huge hue) selects entry 0; anything else past 5 leaves the table.
  switch(std::uint32_t(sector)&0x3fffffffu) {
  case 0:{const auto v=value();w32(m,out,v);w32(m,out+4u,t);w32(m,out+8u,p);break;}
  case 1:{w32(m,out,q);const auto v=value();w32(m,out+4u,v);w32(m,out+8u,p);break;}
  case 2:{w32(m,out,p);const auto v=value();w32(m,out+4u,v);w32(m,out+8u,t);break;}
  case 3:{w32(m,out,p);w32(m,out+4u,q);const auto v=value();w32(m,out+8u,v);break;}
  case 4:{w32(m,out,t);w32(m,out+4u,p);const auto v=value();w32(m,out+8u,v);break;}
  case 5:{const auto v=value();w32(m,out,v);w32(m,out+4u,p);w32(m,out+8u,q);break;}
  default:refuse("134510","hue sector outside the six-entry switch (hue outside [0, 1))");
  }
}

std::uint32_t color_count(const M& m,std::uint32_t body) {
  // MUL AAAAAAABh / SHR 3 / SAR 1 of the sign-extended word: unsigned / 24.
  return sx16(r16(m,body+0x124u))/24u;
}
std::uint32_t color_base(const M& m,std::uint32_t body){return sx16(r16(m,body+0x126u))+body;}
// BCD80 without its output stores.
std::optional<Rgb> color_channel(const M& m,std::uint32_t object,std::uint32_t channel) {
  const auto body=object_body(m,object);
  const auto colors=color_base(m,body);
  const auto count=color_count(m,body);
  if(s32(channel)<0 || s32(channel)>=s32(count))return std::nullopt;
  const auto at=colors+(count+channel)*12u;
  return Rgb{r32(m,at),r32(m,at+4u),r32(m,at+8u)};
}
std::uint32_t pack(const Rgb& rgb,std::uint16_t control) {
  const auto blue=byte_channel(rgb[2],control);
  const auto green=byte_channel(rgb[1],control);
  const auto red=byte_channel(rgb[0],control);
  return blue|green<<8|red<<16;
}
// C033C/C0347..C0621 (and C0AA3..C0B12): clamp to [0, 1] in place; NaN stays.
std::uint32_t clamp_unit(std::uint32_t value,std::uint32_t mxcsr) {
  if(greater(0,value,mxcsr))return 0;
  return greater(value,one_bits,mxcsr)?one_bits:value;
}
void clamp_color(M& m,std::uint32_t at,std::uint32_t mxcsr) {
  const auto red=clamp_unit(r32(m,at),mxcsr);
  const auto green_in=r32(m,at+4u);w32(m,at,red);
  const auto green=clamp_unit(green_in,mxcsr);
  const auto blue_in=r32(m,at+8u);w32(m,at+4u,green);
  w32(m,at+8u,clamp_unit(blue_in,mxcsr));
}
void copy_color(M& m,std::uint32_t to,std::uint32_t from) {
  w32(m,to,r32(m,from));w32(m,to+4u,r32(m,from+4u));w32(m,to+8u,r32(m,from+8u));
}
// C0378..C05B9: the position-hashed blend of the selected permutation's two
// colors (1343E0 twice, the hue wrap, 134510 into `out`).
void blend_permutation(M& m,std::uint32_t body,std::uint32_t definition,std::uint32_t row,std::int32_t permutations,
    std::uint32_t index,std::uint32_t out,std::uint16_t control,std::uint32_t mxcsr) {
  const float index_value=float(std::int32_t(index));   // FILD/FSTP dword: exact below 2^24
  const auto first=store_single(fmod_by_one(position_hash(f(r32(m,body+0x64u)),f(r32(m,body+0x68u)),
      f(r32(m,body+0x6cu)),index_value,control)),control);
  const auto second=store_single(fmod_by_one(secondary_hash(f(r32(m,body+0x68u)),index_value,control)),control);
  std::uint32_t variant_name=0;
  const auto variant=r8(m,body+0xb1u);
  if(variant!=0xffu) {
    const auto model_tag=r32(m,definition+0x38u);
    if(model_tag!=none)variant_name=r32(m,sx8(variant)*0x38u+r32(m,tag_data(m,model_tag)+0x54u));
  }
  const auto rows=r32(m,row+4u);
  std::uint32_t selected=0,total=0;std::int32_t last=-1;
  for(std::int32_t k=0;k<permutations;++k) {
    const auto at=rows+std::uint32_t(k)*0x20u;
    const auto name=r32(m,at+0x1cu);
    if(!name || name==variant_name) {
      total=add(r32(m,at),total,mxcsr);selected|=1u<<(k&31);last=k;
    }
  }
  if(!selected || !greater(total,0,mxcsr))return;
  const auto threshold=mul(first,total,mxcsr);
  std::uint32_t running=0;
  for(std::int32_t k=0;k<permutations;++k) {
    if(!(selected&(1u<<(k&31))))continue;
    const auto at=rows+std::uint32_t(k)*0x20u;
    running=add(r32(m,at),running,mxcsr);
    if(k!=last && !greater_equal(running,threshold,mxcsr))continue;
    const auto weight=sub(one_bits,second,mxcsr);
    const auto ignore=[](std::uint32_t,std::uint32_t){};
    auto from=rgb_to_hsv(Rgb{r32(m,at+4u),r32(m,at+8u),r32(m,at+0xcu)},mxcsr,ignore);
    auto to=rgb_to_hsv(Rgb{r32(m,at+0x10u),r32(m,at+0x14u),r32(m,at+0x18u)},mxcsr,ignore);
    if(hue_gap_above_half(from.hue,to.hue,control)) {
      if(greater(to.hue,from.hue,mxcsr))from.hue=add(from.hue,one_bits,mxcsr);
      else to.hue=add(to.hue,one_bits,mxcsr);
    }
    auto hue=add(mul(from.hue,weight,mxcsr),mul(to.hue,second,mxcsr),mxcsr);
    if(greater(hue,one_bits,mxcsr))hue=sub(hue,one_bits,mxcsr);
    const auto saturation=add(mul(from.saturation,weight,mxcsr),mul(to.saturation,second,mxcsr),mxcsr);
    const auto value=add(mul(from.value,weight,mxcsr),mul(to.value,second,mxcsr),mxcsr);
    hsv_to_rgb(m,out,hue,saturation,[value]{return value;},mxcsr);
    return;
  }
}

// 170930's candidate rows (0x1C bytes on the caller's stack).
struct Candidate {std::uint32_t permutation,region,id,flags,effect,extra,weight;};
constexpr std::size_t candidate_capacity=32;   // 1700E0's frame holds 32 rows
// 170930 for 1700E0's call (filter -1, id 0, flags 0, random, capacity 32):
// one candidate per variant region row, picked by a seeded weighted draw.
void variant_candidates(M& m,std::uint32_t source,std::vector<Candidate>& out,std::uint16_t control,std::uint32_t mxcsr) {
  for(std::int32_t k=0;k<s32(r32(m,source+8u));++k) {
    const auto row=r32(m,source+0xcu)+std::uint32_t(k)*0x20u;
    std::uint32_t id=0,flags=0,effect=none,extra=0,permutation;
    if(s32(r32(m,row+0xcu))>0) {
      const auto seed=r32(m,random_seed_pointer);
      const auto state=r32(m,seed)*0x19660du+0x3c6ef35fu;
      w32(m,seed,state);
      const auto draw=random_unit(state>>16,control);
      const auto entries=s32(r32(m,row+0xcu));
      std::uint32_t running=0;bool found=false;
      if(entries>0) {
        const auto first=r32(m,row+0x10u);
        for(std::int32_t j=0;j<entries;++j) {
          const auto entry=first+std::uint32_t(j)*0x18u;
          const auto sum=add(r32(m,entry+0x14u),running,mxcsr);
          if(greater(sum,draw,mxcsr)) {
            extra=r32(m,entry+0x10u);permutation=sx8(r8(m,entry+4u));effect=r32(m,entry+0xcu);
            id=r16(m,entry+6u);flags=r8(m,entry+5u);found=true;break;
          }
          running=sum;
        }
      }
      if(!found)permutation=sx8(r8(m,row+4u));
    } else permutation=sx8(r8(m,row+4u));   // id and flags are both zero
    if(out.size()<candidate_capacity)
      out.push_back({permutation,std::uint32_t(k),id,flags&0xffu,effect,extra,r32(m,row+8u)});
  }
}
// 170B40: weighted pick among `candidates` (one seeded draw for two or more).
std::int32_t pick_candidate(M& m,const std::vector<Candidate>& candidates,std::uint16_t control,std::uint32_t mxcsr) {
  const auto count=std::int32_t(candidates.size());
  if(!count)return -1;
  if(count==1)return 0;
  std::uint32_t total=0;std::int32_t n=0;
  for(;n+4<=count;n+=4) {
    auto sum=add(candidates[n].weight,total,mxcsr);
    sum=add(sum,candidates[n+1].weight,mxcsr);sum=add(sum,candidates[n+2].weight,mxcsr);
    total=add(sum,candidates[n+3].weight,mxcsr);
  }
  for(;n<count;++n)total=add(total,candidates[n].weight,mxcsr);
  const auto seed=r32(m,random_seed_pointer);
  const auto state=r32(m,seed)*0x19660du+0x3c6ef35fu;
  w32(m,seed,state);
  const auto threshold=random_scaled(state>>16,total,control);
  std::uint32_t running=0;
  for(std::int32_t k=0;;++k) {
    running=add(candidates[k].weight,running,mxcsr);
    if(greater_equal(running,threshold,mxcsr) || k==count-1)return k;
  }
}
std::int32_t region_count(const M& m,std::uint32_t body) {
  return std::int32_t(std::int16_t(r16(m,body+0x118u)))/10;   // IMUL 66666667h/SAR 2: truncating
}
}

std::uint32_t color_rgb_to_hsv(M& m,std::uint32_t rgb,std::uint32_t hsv,std::uint32_t mxcsr) {
  // The original re-reads the RGB words between its comparisons; nothing
  // stores before the first [hsv] store, so one read of each is equivalent
  // unless `hsv` overlaps `rgb`, which the factory never passes.
  const Rgb color{r32(m,rgb),r32(m,rgb+4u),r32(m,rgb+8u)};
  if(hsv<rgb+12u && rgb<hsv+12u)refuse("1343E0","overlapping RGB and HSV operands");
  rgb_to_hsv(color,mxcsr,[&](std::uint32_t offset,std::uint32_t value){w32(m,hsv+offset,value);});
  return hsv;
}

std::uint32_t color_hsv_to_rgb(M& m,std::uint32_t rgb,std::uint32_t hsv,std::uint32_t mxcsr) {
  const auto saturation=r32(m,hsv+4u),hue=r32(m,hsv);
  hsv_to_rgb(m,rgb,hue,saturation,[&]{return r32(m,hsv+8u);},mxcsr);
  return rgb;
}

std::uint32_t color_pack(const M& m,std::uint32_t rgb,std::uint16_t x87_control) {
  return pack(Rgb{r32(m,rgb),r32(m,rgb+4u),r32(m,rgb+8u)},x87_control);
}

bool object_color_channel(M& m,std::uint32_t object,std::uint32_t channel,std::uint32_t out) {
  const auto color=color_channel(m,object,channel);
  if(!color)return false;
  w32(m,out,(*color)[0]);w32(m,out+4u,(*color)[1]);w32(m,out+8u,(*color)[2]);
  return true;
}

void object_color_publish(M& m,std::uint32_t object,bool force,std::uint16_t x87_control) {
  const auto body=object_body(m,object);
  const auto handle=r32(m,body+0xccu);
  if(handle==none)return;
  if(!force && !(r8(m,tag_data(m,r32(m,body))+0x1cu)&1u))return;
  const auto row=r32(m,r32(m,render_rows)+0x44u)+((handle&0xffffu)<<8);
  w8(m,row+0xb8u,0);
  for(std::uint32_t channel=0;channel<4;++channel) {
    const auto color=color_channel(m,object,channel);
    if(!color)continue;
    w32(m,row+0xa8u+channel*4u,pack(*color,x87_control));
    w8(m,row+0xb8u,std::uint8_t(r8(m,row+0xb8u)+1u));
  }
}

void object_change_colors_initialize(M& m,std::uint32_t object,std::uint32_t mask,std::uint32_t colors,
    std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  const auto count=color_count(m,body);
  const auto base=color_base(m,body);
  for(std::uint32_t index=0;s32(index)<s32(count);++index) {
    const auto current=base+index*12u;
    const auto row=r32(m,definition+0xb0u)+index*16u;
    if(mask&(1u<<(index&31u))) {
      copy_color(m,current,colors+index*12u);
    } else {
      copy_color(m,current,r32(m,default_color_pointer));
      const auto permutations=s32(r32(m,row));
      if(permutations>0)blend_permutation(m,body,definition,row,permutations,index,current,x87_control,mxcsr);
    }
    clamp_color(m,current,mxcsr);
    copy_color(m,base+(count+index)*12u,current);
  }
  object_color_publish(m,object,true,x87_control);
}

std::uint32_t object_function_colors(M& m,std::uint32_t object,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  if(!(r8(m,definition+0x1cu)&1u))return 0;
  const auto base=color_base(m,body);
  const auto count=color_count(m,body);
  if(s32(count)<=0)return 0;
  for(std::uint32_t index=0;index<count;++index) {
    const auto row=r32(m,definition+0xb0u)+index*16u;
    const auto current=base+(count+index)*12u;
    copy_color(m,current,base+index*12u);
    // C09D0..C0A2D: each entry's function (+24) value, BCB70 into a stack
    // slot (0.0 when inactive), blends its colors +8/+14 into the current
    // color (134730: EAX flags +4, ECX +8, ESI +14, EDI current, XMM5 value).
    for(std::int32_t k=0;k<s32(r32(m,row+8u));++k) {
      const auto entry=r32(m,row+0xcu)+std::uint32_t(k)*0x28u;
      const auto name=r32(m,entry+0x24u);
      if(!name)continue;
      OriginalOutput value{};
      const auto t=object_function_value(m,object,name,value,x87_control,mxcsr)?value.value:0u;
      color_interpolate(m,r32(m,entry+4u),entry+8u,entry+0x14u,current,t,mxcsr);
    }
    // C0A40..C0AA1: each entry's scale (+20) value (0.0 when inactive)
    // multiplies the current color (value * r, then g * value, b * value).
    for(std::int32_t k=0;k<s32(r32(m,row+8u));++k) {
      const auto name=r32(m,r32(m,row+0xcu)+std::uint32_t(k)*0x28u+0x20u);
      if(!name)continue;
      OriginalOutput value{};
      const auto scale=object_function_value(m,object,name,value,x87_control,mxcsr)?value.value:0u;
      w32(m,current,mul(scale,r32(m,current),mxcsr));
      w32(m,current+4u,mul(r32(m,current+4u),scale,mxcsr));
      w32(m,current+8u,mul(r32(m,current+8u),scale,mxcsr));
    }
    clamp_color(m,current,mxcsr);
  }
  return 1;
}

void object_region_bank(M& m,std::uint32_t object,std::uint32_t count_slot,std::uint32_t active_slot,
    std::uint32_t cached_slot,std::uint32_t rows_slot) {
  const auto body=object_body(m,object);
  const auto extent=r16(m,body+0x118u);
  w32(m,count_slot,sx16(extent));
  const auto base=sx16(r16(m,body+0x11au))+body;
  w32(m,count_slot,std::uint32_t(std::int32_t(std::int16_t(extent))/10));
  if(active_slot)w32(m,active_slot,base);
  if(cached_slot)w32(m,cached_slot,r32(m,count_slot)+base);
  if(rows_slot)w32(m,rows_slot,base+r32(m,count_slot)*2u);
}

void model_region_permutations_select(M& m,std::uint32_t model_tag,std::int32_t variant,std::uint32_t states,
    std::uint32_t rows,std::uint32_t mask,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto slot=(model_tag&0xffffu)*16u+r32(m,tag_instances)+8u;
  const auto model=r32(m,slot);
  for(std::uint32_t i=0;s32(i)<s32(r32(m,model+0x70u));++i) {
    const auto at=rows+i*8u;
    w8(m,states+i,0xff);w8(m,at,0xff);w8(m,at+1u,0);w8(m,at+2u,0);w32(m,at+4u,none);
  }
  for(std::uint32_t i=0;s32(i)<s32(r32(m,model+0x70u));++i) {
    const auto region=r32(m,model+0x74u)+i*16u;
    std::vector<Candidate> candidates;
    if(variant!=-1 && model_tag!=none && i*16u!=0xfffffff0u) {
      const auto variant_row=r32(m,r32(m,slot)+0x54u)+std::uint32_t(variant)*0x38u;
      const auto variant_region=sx8(r8(m,variant_row+i+4u));
      if(variant_region!=none) {
        const auto source=r32(m,r32(m,model+0x54u)+std::uint32_t(variant)*0x38u+0x18u)+variant_region*0x14u;
        variant_candidates(m,source,candidates,x87_control,mxcsr);
      }
    }
    if(candidates.empty()) {
      // 1701FB..1703FC (four at a time, then the rest): every unflagged
      // permutation in order, into the 32-row stack array.
      const auto permutations=s32(r32(m,region+8u));
      for(std::int32_t k=0;k<permutations;++k)
        if(!(r8(m,r32(m,region+0xcu)+std::uint32_t(k)*8u+4u)&1u)) {
          if(candidates.size()==candidate_capacity)refuse("1700E0","more than 32 permutation candidates (stack overrun)");
          candidates.push_back({std::uint32_t(k),none,0,0,none,0,one_bits});
        }
      if(candidates.empty() && permutations>0)candidates.push_back({0,none,0,0,none,0,one_bits});
    }
    if(mask&(1u<<(i&31u)))continue;
    const auto chosen=pick_candidate(m,candidates,x87_control,mxcsr);
    if(chosen==-1)continue;
    const auto& c=candidates[std::size_t(chosen)];
    const auto at=rows+i*8u;
    w8(m,states+i,std::uint8_t(c.permutation));
    w8(m,at,std::uint8_t(c.region));
    w8(m,at+2u,std::uint8_t(c.flags));
    w8(m,at+1u,std::uint8_t(c.id));
  }
}

void object_region_effect_restart(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  const auto entries=sx16(r16(m,body+0x11cu))>>3;   // MOVSX then SHR 3
  const auto block=sx16(r16(m,body+0x11eu))+body;
  if(s32(entries)<=0)return;
  for(std::uint32_t n=0;s32(n)<s32(entries);++n) {
    if(r8(m,block+n*8u)!=2u)continue;
    const auto effect=r32(m,block+n*8u+4u);
    if(effect==none)return;
    const auto pool=r32(m,effect_pool);
    const auto index=effect&0xffffu;
    if(s32(index)>=s32(r32(m,pool+0x38u)))return;
    const auto row=r32(m,pool+0x24u)*index+r32(m,pool+0x44u);
    const auto salt=r16(m,row);
    if(!salt || sx16(salt)!=std::uint32_t(s32(effect)>>16))return;
    refuse("17B1F0","restarting a live region-attached effect's locations (17E060)");
  }
}

void object_region_physics_check(M& m,std::uint32_t object,std::span<const std::uint8_t> previous,
    std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,object);
  const auto component=r32(m,body+0xb4u);
  if(component==none)return;
  const auto flags=r32(m,(component&0xffffu)*0xa0u+r32(m,r32(m,havok_components)+0x44u)+4u);
  if(!((flags>>11)&1u))return;
  const auto model=tag_data(m,r32(m,tag_data(m,r32(m,body))+0x38u));
  const auto regions=s32(r32(m,model+0x70u));
  const auto states=sx16(r16(m,body+0x11au))+body;
  for(std::int32_t i=0;i<regions;++i) {
    if(std::size_t(i)>=previous.size())refuse("BC5C0","1CE0E0 reading prior region states past the caller's copy");
    const auto old=previous[std::size_t(i)];
    const auto current=r8(m,states+std::uint32_t(i));
    if(old==current)continue;
    const auto permutations=r32(m,r32(m,model+0x74u)+std::uint32_t(i)*16u+0xcu);
    bool rebuild;
    if(old==0xffu || current==0xffu)
      rebuild=r8(m,permutations+sx8(old==0xffu?current:old)*8u+6u)!=0xffu;
    else rebuild=r8(m,permutations+sx8(old)*8u+6u)!=r8(m,permutations+sx8(current)*8u+6u);
    if(!rebuild)continue;
    // 1CE1C9: 1CC310(object, 0), then return (the havok track's owner; its
    // shape refresh 1D73D0 and removal 1D9510 are still refusals).
    HavokFp fp{x87_control,0,mxcsr};
    havok_component_state_flags(m,object,0,fp);
    return;
  }
}

void object_regions_initialize(M& m,std::uint32_t object,std::uint32_t mask,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  const auto count=region_count(m,body);
  const auto states=sx16(r16(m,body+0x11au))+body;
  const auto cached=states+std::uint32_t(count);
  const auto rows=states+std::uint32_t(count)*2u;
  // The prior states are copied into a 16-byte stack buffer (REP MOVSD/B).
  if(count<0 || count>16)refuse("BC5C0","a region count outside its 16-byte stack copy");
  std::vector<std::uint8_t> previous(static_cast<std::size_t>(count));
  for(std::int32_t i=0;i<count;++i)previous[std::size_t(i)]=r8(m,states+std::uint32_t(i));
  const auto model_tag=r32(m,definition+0x38u);
  if(model_tag==none) {
    for(std::int32_t i=0;i<count;++i) {
      const auto at=rows+std::uint32_t(i)*8u;
      w8(m,states+std::uint32_t(i),0);w8(m,at,0xff);w8(m,at+1u,0);w8(m,at+2u,0);
    }
  } else {
    model_region_permutations_select(m,model_tag,std::int32_t(sx8(r8(m,body+0xb1u))),states,rows,mask,x87_control,mxcsr);
  }
  const auto words=std::uint32_t(count)>>2;
  for(std::uint32_t n=0;n<words;++n)w32(m,cached+n*4u,r32(m,states+n*4u));
  for(std::uint32_t n=words*4u;n<std::uint32_t(count);++n)w8(m,cached+n,r8(m,states+n));
  object_region_effect_restart(m,object);
  object_region_physics_check(m,object,previous,x87_control,mxcsr);
}

// ---- 1D3380 animation manager setup ------------------------------------------
// The 1D5320 subtree belongs to the weapon track (weapon_animation); 1D3380's
// own record stores are here.
namespace {
constexpr std::uint32_t label_any=0x7000001u;
struct Store {std::uint8_t offset,width;std::uint32_t value;};
constexpr std::uint32_t F=0xffffffffu;
// 1D3380's initializer for the three channels (ordered per channel).
constexpr Store setup_channels[]={
  {0,4,F},{4,2,F},{6,2,F},{8,4,F},{0xc,1,F},{0xd,1,F},{0xe,2,F},{0x1c,4,0},{0x10,1,0},{0x11,1,0},{0x12,2,0},
  {0x18,4,one_bits},{0x14,2,0},{0x16,2,0},
  {0x20,4,F},{0x24,2,F},{0x26,2,F},{0x28,4,F},{0x2c,1,F},{0x2d,1,F},{0x2e,2,F},{0x3c,4,0},{0x30,1,0},{0x31,1,0},
  {0x32,2,0},{0x38,4,one_bits},{0x34,2,0},{0x36,2,0},
  {0x40,4,F},{0x44,2,F},{0x46,2,F},{0x48,4,F},{0x4c,1,F},{0x4d,1,F},{0x4e,2,F},{0x5c,4,0},{0x50,1,0},{0x51,1,0},
  {0x52,2,0},{0x58,4,one_bits},{0x54,2,0},{0x56,2,0}};
constexpr Store setup_tail[]={
  {0x65,1,0},{0x64,1,0},{0x67,1,0},{0x66,1,1},{0x61,1,0},{0x60,1,0},{0x63,1,0},{0x6e,2,0},{0x80,4,0}};
void apply(M& m,std::uint32_t base,std::span<const Store> stores) {
  for(const auto& s:stores) {
    const auto at=base+s.offset;
    if(s.width==4)w32(m,at,s.value);else if(s.width==2)w16(m,at,std::uint16_t(s.value));else w8(m,at,std::uint8_t(s.value));
  }
}
}

bool animation_manager_setup(M& m,std::uint32_t record,std::uint32_t graph_tag,std::uint32_t model_tag,bool enable,
    std::uint16_t x87_control,std::uint32_t mxcsr) {
  if(model_tag!=none) {
    if(graph_tag==none)return false;
    if(sx16(r16(m,tag_data(m,graph_tag)+0xcu))!=r32(m,tag_data(m,model_tag)+0x78u))return false;
  }
  w8(m,record+0x6cu,std::uint8_t(enable?r8(m,record+0x6cu)|2u:r8(m,record+0x6cu)&0xfdu));
  w32(m,record+0x70u,none);w32(m,record+0x7cu,none);w32(m,record+0x74u,none);w32(m,record+0x78u,none);
  w32(m,record+0x68u,graph_tag);
  apply(m,record,setup_channels);
  const NativeObjectFactoryFp fp{x87_control,mxcsr};
  weapon_animation::manager_set_state(m,record,label_any,label_any,label_any,label_any,0x317u,0x3fu,fp);
  apply(m,record,setup_tail);
  return true;
}

namespace {
// [0x4739F8] widget types (0x38 rows): group tag and creator (TU1.5 .data).
struct WidgetType {std::uint32_t group;const char* creator;};
constexpr WidgetType widget_types[]={{0x616e7421u,"118480"},{0x636c7764u,"119060"},{0x7464746cu,"C5B00"}};
}

void object_widgets_create(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  w32(m,body+0xdcu,none);
  if(s32(r32(m,definition+0x9cu))<=0)return;
  for(std::uint32_t n=0;s32(sx16(std::uint16_t(n)))<s32(r32(m,definition+0x9cu));++n) {
    const auto row=r32(m,definition+0xa0u)+sx16(std::uint16_t(n))*8u;
    const auto group=r32(m,row);
    for(const auto& type:widget_types) {
      if(type.group!=group)continue;
      if(r32(m,row+4u)!=none)refuse(type.creator,"object widget creator (D6E40)");
      break;
    }
  }
}

void object_attachments_create(M& m,std::uint32_t object,std::uint16_t x87_control,std::uint32_t mxcsr) {
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  if(s32(r32(m,definition+0x94u))<=0)return;
  const auto block=sx16(r16(m,body+0x11eu))+body;
  for(std::uint32_t n=0;s32(n)<s32(r32(m,definition+0x94u));++n) {
    const auto row=r32(m,definition+0x98u)+n*0x18u;
    const auto tag=r32(m,row+4u);
    std::uint32_t kind=none;
    if(tag!=none) {
      switch(r32(m,row)) {
      case 0x6c656e73u:case 0x4d475332u:case 0x7464746cu:kind=5;break;   // 'lens' 'MGS2' 'tdtl'
      case 0x636f6e74u:kind=3;break;                                     // 'cont'
      case 0x65666665u:kind=2;break;                                     // 'effe'
      case 0x6c696768u:kind=0;break;                                     // 'ligh'
      case 0x6c736e64u:kind=1;break;                                     // 'lsnd'
      default:break;
      }
    }
    // C0DD0's four creators (the attachments track); EAX stays NONE for the
    // other kinds. Lights and looping sounds flag body +4 (C0DF7, C0E14).
    std::uint32_t handle=none;
    switch(kind) {
    case 0:   // C0DE9: C2260(tag, object, index, row +10, word row +C)
      handle=object_light_attachment_create(m,tag,object,n,r32(m,row+0x10u),r16(m,row+0xcu),x87_control,mxcsr);
      if(handle!=none)w32(m,body+4u,r32(m,body+4u)|0x10u);
      break;
    case 1:   // C0E06: 18BD80(ESI tag, EDI object; index, row +10)
      handle=object_looping_sound_attachment_create(m,tag,object,n,r32(m,row+0x10u));
      if(handle!=none)w32(m,body+4u,r32(m,body+4u)|0x20u);
      break;
    case 2:   // C0E2E: 179540(EDX object; tag, row +10, row +14, word row +C): refuses at 178E30
      handle=object_effect_attachment_create(m,object,tag,r32(m,row+0x10u),r32(m,row+0x14u),r16(m,row+0xcu));
      break;
    case 3:   // C0E3C: 17E940(EBX object, EDI index; tag)
      handle=object_contrail_attachment_create(m,tag,object,n,x87_control,mxcsr);
      break;
    default:break;
    }
    w8(m,block+n*8u,std::uint8_t(kind));
    w32(m,block+n*8u+4u,handle);
  }
}

void object_type_finish(M& m,std::uint32_t object,std::uint16_t x87_control,std::uint32_t mxcsr) {
  if(r8(m,object_row(m,object)+3u)==5u)w32(m,object_body(m,object)+0xdcu,none);
  else object_widgets_create(m,object);
  object_attachments_create(m,object,x87_control,mxcsr);
}
}
