#include "halo2/native_tu15_function_evaluation_extended.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <emmintrin.h>
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned size=4){
  std::array<std::byte,4>b{};m.read(p,std::span(b).first(size));std::uint32_t v{};
  for(unsigned n=0;n<size;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
  NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_{};
public:
  SimdEnvironment(NativeTu15PhysicsFrameFp&fp,NativeTu15FunctionEvaluationExtendedStats*s):fp_(fp),previous_(_mm_getcsr()){
    if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
      if(s)++s->unsupported_fp_profiles;throw std::runtime_error("Native TU1.5 authored function requires its numerical-trap owner");
    }resume();
  }
  void suspend(){if(active_){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);active_=false;}}
  void resume(){_mm_setcsr(fp_.mxcsr);active_=true;}
  ~SimdEnvironment(){suspend();}
};
__m128 scalar(std::uint32_t b){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(b)));}
std::uint32_t bits(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char v;asm volatile("comiss %2,%1;seta %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v!=0;}
__m128 reciprocal_square_root(__m128 v){asm volatile("rsqrtss %1,%0":"=x"(v):"x"(v):"memory");return v;}
__m128 clamp_unit(__m128 v,__m128 one){
  if(greater(_mm_setzero_ps(),v))return _mm_setzero_ps();
  if(greater(v,one))return one;return v;
}
class X87Environment {
  alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;
public:
  X87Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15FunctionEvaluationExtendedStats*s):fp_(fp){
    if(depth>5u||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)){
      if(s)++s->unsupported_fp_profiles;throw std::runtime_error("Native TU1.5 authored power requires its x87 fault owner");}
    asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::array<std::byte,28>native{};std::memcpy(native.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
    std::memcpy(native.data(),&fp.control_word,2);std::memcpy(native.data()+4,&fp.status,2);std::memcpy(native.data()+8,&empty,2);
    asm volatile("fldenv %0"::"m"(native):"memory");
  }
  ~X87Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));
    asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");}
};
bool small_absolute(std::uint32_t v,std::uint32_t epsilon){
  unsigned char small;asm volatile("flds %[v];fabs;flds %[epsilon];fcomip %%st(1),%%st;fstp %%st(0);seta %[out]"
    :[out]"=qm"(small):[v]"m"(v),[epsilon]"m"(epsilon):"cc","memory","st","st(1)");return small!=0;
}
bool small_difference(std::uint32_t a,std::uint32_t b,std::uint32_t epsilon){
  unsigned char small;asm volatile("flds %[a];fsubs %[b];fabs;flds %[epsilon];fcomip %%st(1),%%st;fstp %%st(0);seta %[out]"
    :[out]"=qm"(small):[a]"m"(a),[b]"m"(b),[epsilon]"m"(epsilon):"cc","memory","st","st(1)");return small!=0;
}
// Ordinary native x87 math, not a guest execution loop. The real result stays
// on this kernel's private native x87 stack until its caller consumes it.
void power_on_stack(SourcePhysicsContactMemory&m,std::uint32_t base,std::uint32_t exponent,
    unsigned depth,NativeTu15FunctionEvaluationExtendedStats*stats){
  if(stats)++stats->power_calls;
  const auto unsupported=[&](){if(stats)++stats->unsupported_power_profiles;throw std::runtime_error("Native TU1.5 authored power requires its remaining exceptional CRT owner");};
  if((base&0x7f800000u)==0x7f800000u||(exponent&0x7f800000u)==0x7f800000u)unsupported();
  if((base&0x80000000u)&&(base&0x7fffffffu)&&depth>4u)unsupported();
  std::uint16_t previous;asm volatile("fnstcw %0":"=m"(previous));
  // The normal CRT's alternate-CW precision publication has a distinct owner.
  if(previous!=0x27fu)unsupported();
  std::uint64_t base_double,exponent_double;
  asm volatile("flds %[base];flds %[exponent];fxch %%st(1);fstpl %[bd];fstl %[ed]"
    :[bd]"=m"(base_double),[ed]"=m"(exponent_double):[base]"m"(base),[exponent]"m"(exponent):"memory","st","st(1)");
  asm volatile("fldl %0"::"m"(base_double):"memory","st");
  bool negate=false;
  if((base&0x7fffffffu)==0u){
    asm volatile("fstp %%st(0)":::"memory","st");
    if((exponent&0x7fffffffu)==0u){asm volatile("fstp %%st(0);fld1":::"memory","st");return;}
    if(exponent&0x80000000u)unsupported();
    // Zero-base sign depends on the same genuine integer/odd classification.
    std::uint16_t status;asm volatile("fld %%st(0);frndint;fcomp %%st(1);fnstsw %%ax":"=a"(status)::"memory","st","st(1)");
    if(status&0x4000u){
      std::array<std::byte,8>raw{};m.read(0x473360u,raw);std::uint64_t half{};std::memcpy(&half,raw.data(),8);
      asm volatile("fmull %[half];fld %%st(0);frndint;fcompp;fnstsw %%ax":"=a"(status):[half]"m"(half):"memory","st","st(1)");
      negate=(base&0x80000000u)&&!(status&0x4000u);
    }else asm volatile("fstp %%st(0)":::"memory","st");
    asm volatile("fldz":::"memory","st");if(negate)asm volatile("fchs":::"memory","st");return;
  }
  if(base&0x80000000u){
    std::uint16_t status;asm volatile("fld %%st(1);fld %%st(0);frndint;fcomp %%st(1);fnstsw %%ax":"=a"(status)::"memory","st","st(1)","st(2)");
    if(!(status&0x4000u))unsupported();
    std::array<std::byte,8>raw{};m.read(0x473360u,raw);std::uint64_t half{};std::memcpy(&half,raw.data(),8);
    asm volatile("fmull %[half];fld %%st(0);frndint;fcompp;fnstsw %%ax":"=a"(status):[half]"m"(half):"memory","st","st(1)","st(2)");
    negate=!(status&0x4000u);asm volatile("fchs":::"memory","st");
  }
  // GAS's AT&T fsub spelling emits DC E1, the source's Intel FSUBR ST1,ST0.
  asm volatile("fyl2x;fld %%st(0);frndint;fsub %%st(0),%%st(1);fxch %%st(1);fchs;f2xm1;fld1;faddp;fscale;fstp %%st(1)"
    :::"memory","st","st(1)","st(2)");
  if(negate)asm volatile("fchs":::"memory","st");
  const auto global=word(m,0x48d4f4u);
  if(!global){std::uint64_t rounded;asm volatile("fstl %0":"=m"(rounded)::"memory","st");
    const auto magnitude=rounded&0x7ff0000000000000ull;
    if(magnitude==0u||magnitude==0x7ff0000000000000ull)unsupported();}
}
#endif
}
std::uint32_t native_tu15_evaluate_authored_function_extended(
    SourcePhysicsContactMemory&m,std::uint32_t reference,std::uint32_t first,
    std::uint32_t second,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    NativeTu15FunctionEvaluationExtendedStats*stats){
  if(stats)++stats->calls;
  const auto blob=word(m,reference+4u);if(!blob){if(stats)++stats->null_functions;return 0;}
  if(std::bit_cast<std::int32_t>(word(m,reference))<=0){if(stats)++stats->empty_functions;return 0;}
  const auto kind=word(m,blob,1);if(kind>10u){if(stats)++stats->invalid_kinds;return 0;}
  if(stats)++stats->kinds[kind];
  if(kind==6u||kind==8u)return 0;
  if(kind==1u&&!(word(m,blob+1u,1)&1u))return 0;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  SimdEnvironment environment(fp,stats);const auto x=scalar(first),blend=scalar(second);const auto data=blob+0x14u;
  __m128 result=_mm_setzero_ps();
  const auto curve=[&](bool periodic,std::uint32_t type,std::uint32_t value){
    environment.suspend();std::uint32_t out;
    try{out=periodic?native_tu15_evaluate_periodic_curve(m,std::uint16_t(type),value,fp,depth,stats?&stats->curves:nullptr):native_tu15_evaluate_transition_curve(m,std::uint16_t(type),value,fp,depth,stats?&stats->curves:nullptr);}
    catch(...){environment.resume();throw;}
    environment.resume();return scalar(out);
  };
  if(kind==0u)result=x;
  else if(kind==1u)result=blend;
  else if(kind==2u){
    auto sampled=curve(false,word(m,blob+2u,1),first);
    auto range=scalar(word(m,data+4u));range=sub(range,scalar(word(m,data)));
    const bool two=(word(m,blob+1u,1)&1u)!=0;
    range=mul(range,sampled);range=add(range,scalar(word(m,data)));result=range;
    if(two){
      sampled=curve(false,word(m,blob+3u,1),first);
      auto upper=scalar(word(m,data+12u));upper=sub(upper,scalar(word(m,data+8u)));
      upper=mul(upper,sampled);upper=add(upper,scalar(word(m,data+8u)));
      auto lower=scalar(word(m,0x46a0d0u));lower=sub(lower,blend);lower=mul(lower,result);
      upper=mul(upper,blend);result=add(upper,lower);
    }
  }else if(kind==3u){
    auto phase=mul(scalar(word(m,data)),x);const auto type=word(m,blob+2u,1);
    phase=add(phase,scalar(word(m,data+4u)));auto sampled=curve(true,type,bits(phase));
    auto range=scalar(word(m,data+12u));range=sub(range,scalar(word(m,data+8u)));
    const bool two=(word(m,blob+1u,1)&1u)!=0;
    range=mul(range,sampled);range=add(range,scalar(word(m,data+8u)));result=range;
    if(two){
      phase=mul(scalar(word(m,data+16u)),x);const auto next=word(m,blob+3u,1);
      phase=add(phase,scalar(word(m,data+20u)));sampled=curve(true,next,bits(phase));
      auto upper=scalar(word(m,data+28u));upper=sub(upper,scalar(word(m,data+24u)));
      upper=mul(upper,sampled);upper=add(upper,scalar(word(m,data+24u)));
      auto lower=scalar(word(m,0x46a0d0u));lower=sub(lower,blend);lower=mul(lower,result);
      upper=mul(upper,blend);result=add(upper,lower);
    }
  }else if(kind==4u){
    result=scalar(word(m,data+16u));const bool two=(word(m,blob+1u,1)&1u)!=0;
    result=mul(result,x);result=add(result,scalar(word(m,data+20u)));
    if(two){
      auto upper=mul(scalar(word(m,data+40u)),x);upper=add(upper,scalar(word(m,data+44u)));
      auto lower=scalar(word(m,0x46a0d0u));lower=sub(lower,blend);lower=mul(lower,result);
      upper=mul(upper,blend);result=add(upper,lower);
    }
  }else if(kind==5u){
    auto one=scalar(word(m,0x46a0d0u));
    const auto ramp=[&](std::uint32_t at,unsigned n,__m128 limit){
      auto v=sub(x,scalar(word(m,at+0x24u+n*4u)));v=mul(v,scalar(word(m,at+0x34u+n*4u)));
      return clamp_unit(v,limit);
    };
    const auto a=ramp(data,0,one),b=ramp(data,1,one),c=ramp(data,2,one);
    result=scalar(word(m,data+0x4cu));const bool two=(word(m,blob+1u,1)&1u)!=0;
    result=mul(result,c);auto term=mul(scalar(word(m,data+0x48u)),b);result=add(result,term);
    term=mul(scalar(word(m,data+0x44u)),a);result=add(result,term);result=add(result,scalar(word(m,data+0x40u)));
    if(two){
      one=scalar(word(m,0x46a0d0u));const auto at=data+0x50u;
      const auto aa=ramp(at,0,one),bb=ramp(at,1,one),cc=ramp(at,2,one);
      auto upper=mul(scalar(word(m,at+0x4cu)),cc);auto q=mul(scalar(word(m,at+0x48u)),bb);
      const auto coefficient=scalar(word(m,at+0x44u));upper=add(upper,q);q=mul(coefficient,aa);upper=add(upper,q);
      upper=add(upper,scalar(word(m,at+0x40u)));one=sub(one,blend);one=mul(one,result);
      upper=mul(upper,blend);result=add(upper,one);
    }
  }else if(kind==9u){
    X87Environment native_x87(fp,depth,stats);
    const auto power_value=[&](std::uint32_t at){
      const auto exponent=word(m,at+8u),epsilon=word(m,0x46a0ecu);
      bool shortcut=small_absolute(exponent,epsilon);
      if(!shortcut&&greater(_mm_setzero_ps(),scalar(exponent))){const auto limit=word(m,0x46a0ecu);shortcut=small_absolute(first,limit);}
      if(shortcut)return scalar(word(m,0x46a0d0u));
      power_on_stack(m,first,exponent,depth,stats);
      const auto upper=word(m,at+4u),lower=word(m,at);std::uint32_t out;
      asm volatile("flds %[upper];fsubs %[lower];fmulp"::[upper]"m"(upper),[lower]"m"(lower):"memory","st","st(1)");
      const auto base=word(m,at);asm volatile("fadds %[base];fstps %[out]" :[out]"=m"(out):[base]"m"(base):"memory","st");
      return scalar(out);
    };
    result=power_value(data);
    if(word(m,blob+1u,1)&1u){auto upper=power_value(data+12u);auto one=scalar(word(m,0x46a0d0u));one=sub(one,blend);one=mul(one,result);upper=mul(upper,blend);result=add(one,upper);}
  }else if(kind==7u||kind==10u){
    // Curve-distance preprocessing is native DATA and native scalar/x87 math.
    // Source second-layer early exits retain the current first-layer input.
    auto parameter=x;
    const auto control_parameter=[&](unsigned layer,__m128 current){
      const auto point=[&](unsigned index){if(stats)++stats->control_point_queries;return native_tu15_authored_function_control_point(m,reference,layer,index);};
      const auto a=point(0),b=point(1),c=point(2),d=point(3);
      auto dy=scalar(word(m,c+4u));dy=sub(dy,scalar(word(m,d+4u)));
      auto dx=scalar(word(m,c));dx=sub(dx,scalar(word(m,d)));
      auto yy=mul(dy,dy);dx=mul(dx,dx);const auto distance=add(dx,yy);const auto epsilon=word(m,0x46a0ecu);
      if(!greater(distance,scalar(epsilon)))return current;
      dy=scalar(word(m,a+4u));dy=sub(dy,scalar(word(m,b+4u)));
      dx=scalar(word(m,a));dx=sub(dx,scalar(word(m,b)));
      yy=mul(dy,dy);dx=mul(dx,dx);const auto first_distance=add(dx,yy);
      X87Environment native_x87(fp,depth,stats);
      if(small_absolute(bits(first_distance),word(m,0x46a0ecu)))return scalar(word(m,0x46a0d0u));
      if(small_difference(bits(first_distance),bits(distance),word(m,0x46a0ecu)))return current;
      const auto signed_bits=std::bit_cast<std::int32_t>(bits(first_distance));
      const auto approximate_root=std::uint32_t(signed_bits>>1u)+0x1fc00000u;
      const auto inverse=bits(reciprocal_square_root(distance));std::uint32_t exponent;
      asm volatile("flds %[inverse];fmuls %[root];fstps %[exponent]" :[exponent]"=m"(exponent):[inverse]"m"(inverse),[root]"m"(approximate_root):"memory","st");
      power_on_stack(m,first,exponent,depth,stats);std::uint32_t out;
      asm volatile("fsts %0":"=m"(out)::"memory","st");
      const auto zero=word(m,0x46a0e8u);unsigned char negative;
      asm volatile("flds %[zero];fcomip %%st(1),%%st;fstp %%st(0);seta %[out]" :[out]"=qm"(negative):[zero]"m"(zero):"cc","memory","st","st(1)");
      if(negative)return _mm_setzero_ps();const auto one=scalar(word(m,0x46a0d0u));return greater(scalar(out),one)?one:scalar(out);
    };
    if(kind==10u)parameter=control_parameter(0,parameter);
    auto quadratic=scalar(word(m,blob+0x38u)),linear=scalar(word(m,blob+0x3cu));
    const bool two=(word(m,blob+1u,1)&1u)!=0;
    auto square=mul(parameter,parameter);quadratic=mul(quadratic,square);linear=mul(linear,parameter);result=add(quadratic,linear);
    auto cubic=scalar(word(m,blob+0x34u));auto cube=mul(square,parameter);cubic=mul(cubic,cube);
    result=add(result,cubic);result=add(result,scalar(word(m,blob+0x40u)));
    if(two){
      if(word(m,blob,1)==10u){parameter=control_parameter(1,parameter);square=mul(parameter,parameter);cube=mul(square,parameter);}
      auto upper=mul(scalar(word(m,blob+0x68u)),square);auto q=mul(scalar(word(m,blob+0x6cu)),parameter);
      auto c=scalar(word(m,blob+0x64u));upper=add(upper,q);auto one=scalar(word(m,0x46a0d0u));
      c=mul(c,cube);upper=add(upper,c);upper=add(upper,scalar(word(m,blob+0x70u)));
      one=sub(one,blend);one=mul(one,result);upper=mul(upper,blend);result=add(upper,one);
    }
  }
  if(greater(_mm_setzero_ps(),result)){if(stats)++stats->lower_clamps;return 0;}
  const auto one=word(m,0x46a0d0u);if(greater(result,scalar(one))){if(stats)++stats->upper_clamps;return one;}
  return bits(result);
#else
  throw std::runtime_error("Native TU1.5 authored function SIMD backend unavailable");
#endif
}
} // namespace halo2
