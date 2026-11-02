#include "halo2/native_tu15_function_evaluation.hpp"
#include <bit>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <emmintrin.h>
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned size=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(size));std::uint32_t v{};for(unsigned n=0;n<size;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
  NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
  SimdEnvironment(NativeTu15PhysicsFrameFp&fp,NativeTu15FunctionEvaluationStats*stats):fp_(fp),previous_(_mm_getcsr()){
    if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
      if(stats)++stats->unsupported_fp_profiles;throw std::runtime_error("Native TU1.5 function requires its numerical-trap owner");
    }
    _mm_setcsr(fp.mxcsr);
  }
  ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
bool greater(__m128 left,__m128 right){unsigned char result;asm volatile("comiss %2,%1;seta %0":"=qm"(result):"x"(left),"x"(right):"cc","memory");return result!=0;}
std::uint32_t clamp(SourcePhysicsContactMemory&m,std::uint32_t result,NativeTu15PhysicsFrameFp&fp,NativeTu15FunctionEvaluationStats*stats){
  SimdEnvironment environment(fp,stats);const auto value=scalar(result);
  if(greater(_mm_setzero_ps(),value)){if(stats)++stats->lower_clamps;return 0;}
  const auto one=word(m,0x46a0d0u);
  if(greater(value,scalar(one))){if(stats)++stats->upper_clamps;return one;}
  return result;
}
#endif
}
std::uint32_t native_tu15_evaluate_authored_function(SourcePhysicsContactMemory&m,
    std::uint32_t reference,std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,NativeTu15FunctionEvaluationStats*stats){
  (void)depth;if(stats)++stats->calls;
  const auto blob=word(m,reference+4u);if(!blob){if(stats)++stats->null_functions;return 0;}
  if(std::bit_cast<std::int32_t>(word(m,reference))<=0){if(stats)++stats->empty_functions;return 0;}
  const auto kind=word(m,blob,1);if(kind>10u){if(stats)++stats->invalid_kinds;return 0;}
  std::uint32_t result{};
  if(kind==0){if(stats)++stats->kind_zero;result=first;}
  else if(kind==1){if(stats)++stats->kind_one;if(!(word(m,blob+1u,1)&1u))return 0;result=second;}
  else{if(stats)++stats->unsupported_kinds;throw std::runtime_error("Native TU1.5 authored function kind requires additional native math");}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  return clamp(m,result,fp,stats);
#else
  throw std::runtime_error("Native TU1.5 authored function SIMD backend unavailable");
#endif
}
} // namespace halo2
