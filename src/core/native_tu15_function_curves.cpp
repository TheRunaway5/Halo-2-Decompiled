#include "halo2/native_tu15_function_curves.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <emmintrin.h>
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned size=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(size));std::uint32_t v{};for(unsigned n=0;n<size;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
std::uint64_t wide(const SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,8>b{};m.read(p,b);std::uint64_t v{};for(unsigned n=0;n<8;++n)v|=std::uint64_t(std::to_integer<unsigned>(b[n]))<<(8u*n);return v;}
bool nan(std::uint32_t b){return(b&0x7f800000u)==0x7f800000u&&(b&0x7fffffu);}
bool finite(std::uint32_t b){return(b&0x7f800000u)!=0x7f800000u;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15FunctionCurveStats*stats):fp_(fp),prior_(_mm_getcsr()){
    if(depth>6u||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
      if(stats)++stats->unsupported_fp_profiles;throw std::runtime_error("Native TU1.5 authored curve requires its numerical-fault owner");}
    asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::array<std::byte,28>native{};std::memcpy(native.data(),host_.data(),28);const std::uint16_t empty=0xffffu;std::memcpy(native.data(),&fp.control_word,2);std::memcpy(native.data()+4,&fp.status,2);std::memcpy(native.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
};
__m128 scalar(std::uint32_t b){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(b)));}
std::uint32_t bits(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char v;asm volatile("comiss %2,%1;seta %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v!=0;}
__m128 integer(std::uint32_t v){auto out=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(out):"r"(std::bit_cast<std::int32_t>(v)):"memory");return out;}
void remainder_on_stack(std::uint32_t value,std::uint64_t one,bool value_live=false){
  if(!value_live)asm volatile("flds %0"::"m"(value):"memory","st");
  std::uint16_t previous;const std::uint16_t precise=0x133fu;
  // Normal finite_CIfmod: its canonical precision/masks, native x87 FPREM
  // untilC2 clears, then original CW. Keep the real result live until the
  // caller's source FSTP phase; no private control frame is represented.
  if((value&0x7fffffffu)==0u){
    // Source331ECB classifies both operands before its zero-numerator owner
    // 331F68 discards both and loads +0. FXAM's condition flags are observable.
    asm volatile("fnstcw %[old];fldl %[one];fldcw %[precise];"
      "fxam;fxch %%st(1);fxam;fxch %%st(1);fstp %%st(0);fstp %%st(0);fldz;fldcw %[old]"
      :[old]"=m"(previous):[value]"m"(value),[one]"m"(one),[precise]"m"(precise):"memory","st","st(1)");
    return;
  }
  asm volatile("fnstcw %[old];fldl %[one];fldcw %[precise];fxch %%st(1);"
    "1:fprem;fnstsw %%ax;test $0x400,%%ax;jnz 1b;fstp %%st(1);fldcw %[old]"
    :[old]"=m"(previous):[value]"m"(value),[one]"m"(one),[precise]"m"(precise):"ax","cc","memory","st","st(1)");
}
std::uint32_t pop_float(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
std::uint32_t round_integer(std::uint32_t value){std::uint32_t result;asm volatile("flds %1;fistpl %0":"=m"(result):"m"(value):"memory","st");return result;}
#endif
}
std::uint32_t native_tu15_evaluate_transition_curve(SourcePhysicsContactMemory&m,
    std::uint16_t type,std::uint32_t parameter,NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15FunctionCurveStats*stats){
  if(stats)++stats->transition_calls;
  if(type>7u){if(stats)++stats->unsupported_types;throw std::runtime_error("Native TU1.5 transition curve requires an authenticated authored type");}
  if(type&&nan(parameter)){if(stats)++stats->unsupported_inputs;throw std::runtime_error("Native TU1.5 transition NaN curve requires its native CRT owner");}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp,depth,stats);auto value=scalar(parameter),zero=_mm_setzero_ps();
  if(greater(zero,value))value=zero;else{const auto one=scalar(word(m,0x46a0d0u));if(greater(value,one))value=one;}
  if(!type)return bits(value);
  const auto scaled=mul(value,scalar(word(m,0x46a43cu)));const auto table=0x4782a8u+std::uint32_t(type)*1024u;
  const auto scaled_bits=bits(scaled);asm volatile("flds %0"::"m"(scaled_bits):"memory","st");
  const auto modulus=wide(m,0x46a1d0u);remainder_on_stack(scaled_bits,modulus,true);if(stats)++stats->native_remainders;
  const auto shifted=sub(scaled,scalar(word(m,0x46a178u)));const auto fraction=scalar(pop_float());const auto rounded=round_integer(bits(shifted));const auto one=scalar(word(m,0x46a0d0u));
  __m128 result;
  if(std::uint16_t(rounded)==1023u){const auto sample=word(m,table+1023u,1);result=mul(integer(sample),scalar(word(m,0x46a0c4u)));}
  else{
    const auto reciprocal=scalar(word(m,0x46a0c4u));const auto index=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(rounded)));
    const auto first=word(m,table+std::uint32_t(index),1),second=word(m,table+std::uint32_t(index)+1u,1);
    const auto first_term=mul(mul(integer(first),reciprocal),sub(one,fraction));const auto second_term=mul(mul(integer(second),reciprocal),fraction);result=add(first_term,second_term);
  }
  if(stats)++stats->table_samples;
  if(greater(_mm_setzero_ps(),result))return 0;
  if(greater(result,one))return bits(one);return bits(result);
#else
  throw std::runtime_error("Native TU1.5 transition curve numeric backend unavailable");
#endif
}
std::uint32_t native_tu15_evaluate_periodic_curve(SourcePhysicsContactMemory&m,
    std::uint16_t type,std::uint32_t parameter,NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15FunctionCurveStats*stats){
  if(stats)++stats->periodic_calls;if(!type)return word(m,0x46a0d0u);
  if(type>11u){if(stats)++stats->unsupported_types;throw std::runtime_error("Native TU1.5 periodic curve requires an authenticated authored type");}
  if(!finite(parameter)){if(stats)++stats->unsupported_inputs;throw std::runtime_error("Native TU1.5 periodic nonfinite curve requires its native CRT owner");}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp,depth,stats);const auto scaled=mul(scalar(parameter),scalar(word(m,0x46a590u)));
  if(!finite(bits(scaled))){if(stats)++stats->unsupported_inputs;throw std::runtime_error("Native TU1.5 periodic overflow requires its native CRT owner");}
  const auto modulus=wide(m,0x46a1d0u);remainder_on_stack(bits(scaled),modulus);if(stats)++stats->native_remainders;const auto fraction=scalar(pop_float());const auto rounded=round_integer(bits(sub(scaled,fraction)));const auto reciprocal=scalar(word(m,0x46a0c4u));
  const auto table=0x4752a8u+std::uint32_t(type)*1024u,index=rounded&1023u,first=word(m,table+index,1),second=word(m,table+((index+1u)&1023u),1);
  const auto first_integer=integer(first),second_integer=integer(second);
  auto first_value=mul(first_integer,reciprocal),second_value=mul(second_integer,reciprocal);
  if((1u<<(type&31u))&0xc0u){
    const auto high=greater(first_value,scalar(word(m,0x46a13cu)));const auto one=scalar(word(m,0x46a0d0u));
    if(high){if(greater(scalar(word(m,0x46a130u)),second_value))second_value=add(second_value,one);}
    auto result=add(mul(sub(one,fraction),first_value),mul(second_value,fraction));if(greater(result,one))result=sub(result,one);if(stats)++stats->table_samples;return bits(result);
  }
  const auto one=scalar(word(m,0x46a0d0u));const auto result=add(mul(sub(one,fraction),first_value),mul(second_value,fraction));if(stats)++stats->table_samples;return bits(result);
#else
  throw std::runtime_error("Native TU1.5 periodic curve numeric backend unavailable");
#endif
}
} // namespace halo2
