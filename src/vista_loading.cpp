#include "halo2/vista_loading.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
float scalar(std::uint32_t value) { return std::bit_cast<float>(value); }
std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }

class FloatingPointScope {
  std::fenv_t saved;
  unsigned mxcsr;
public:
  explicit FloatingPointScope(const VistaLoadingFloatingPoint& context)
      :mxcsr(_mm_getcsr()) {
    if((context.x87_control_word&0x3f)!=0x3f ||
       (context.x87_control_word&0x300)==0x100 ||
       (context.mxcsr&0x1f80)!=0x1f80 || (context.mxcsr&0xffff0000))
      throw std::runtime_error("Original Vista loading FP exception/precision context unsupported");
    if(std::fegetenv(&saved)!=0)
      throw std::runtime_error("Original Vista loading FP context unavailable");
    _mm_setcsr(context.mxcsr);
    asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope() { std::fesetenv(&saved);_mm_setcsr(mxcsr); }
};

float add(float a,float b) { return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float subtract(float a,float b) { return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float divide(float a,float b) { return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float integer(std::int32_t value) { return _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),value)); }
bool above(float a,float b) {
  unsigned char result;
  asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc");
  return result;
}
bool above_equal(float a,float b) {
  unsigned char result;
  asm volatile("comiss %2,%1; setae %0":"=qm"(result):"x"(a),"x"(b):"cc");
  return result;
}
bool below_equal_or_unordered(float a,float b) {
  unsigned char result;
  asm volatile("comiss %2,%1; setbe %0":"=qm"(result):"x"(a),"x"(b):"cc");
  return result;
}
long double load(float value) {
  long double result;
  asm volatile("flds %1; fstpt %0":"=m"(result):"m"(value):"st");
  return result;
}
float spill(long double value) {
  float result;
  asm volatile("fldt %1; fstps %0":"=m"(result):"m"(value):"st");
  return result;
}
float copy(float value) { return spill(load(value)); }
long double add_extended(long double a,float b) {
  long double result;
  asm volatile("fldt %1; fadds %2; fstpt %0":"=m"(result):"m"(a),"m"(b):"st");
  return result;
}
long double multiply_extended(long double a,float b) {
  long double result;
  asm volatile("fldt %1; fmuls %2; fstpt %0":"=m"(result):"m"(a),"m"(b):"st");
  return result;
}
long double divide_extended(long double a,float b) {
  long double result;
  asm volatile("fldt %1; fdivs %2; fstpt %0":"=m"(result):"m"(a),"m"(b):"st");
  return result;
}
long double unsigned_clock(std::uint32_t value) {
  const auto signed_value=std::bit_cast<std::int32_t>(value);
  long double result;
  asm volatile("fildl %1; fstpt %0":"=m"(result):"m"(signed_value):"st");
  // 647FBA/66A0CC/66A59B add the literal binary32 2^32 after signed FILD.
  if(signed_value<0)result=add_extended(result,scalar(0x4f800000));
  return result;
}
float elapsed_seconds(std::uint32_t origin,std::uint32_t now) {
  return spill(multiply_extended(unsigned_clock(now-origin),scalar(0x3a83126f)));
}
long double target(float secondary,float normalized) {
  const float secondary_weight=scalar(0x3e99999a),map_weight=scalar(0x3f333333);
  long double result;
  asm volatile("flds %1; fmuls %3; flds %2; fmuls %4; faddp; fstpt %0"
      :"=m"(result):"m"(secondary),"m"(normalized),"m"(secondary_weight),"m"(map_weight)
      :"st","st(1)");
  return result;
}
bool below_or_unordered(long double target_value,float progress) {
  unsigned char result;
  asm volatile("flds %2; fldt %1; fcomip %%st(1),%%st; fstp %%st(0); setb %0"
      :"=qm"(result):"m"(target_value),"m"(progress):"cc","st","st(1)");
  return result;
}
std::uint32_t packed_channel(float value) {
  const float scale=scalar(0x437f0000);
  std::int32_t result;
  asm volatile("flds %1; fmuls %2; fistpl %0":"=m"(result):"m"(value),"m"(scale):"st");
  return std::bit_cast<std::uint32_t>(result);
}
std::uint32_t packed_color(float gain) {
  // Original 47602F does not clamp or mask each integer before these ORs.
  const auto alpha=packed_channel(scalar(0x3f800000));
  const auto red=packed_channel(gain),green=packed_channel(gain),blue=packed_channel(gain);
  return (alpha<<24)|(red<<16)|(green<<8)|blue;
}
#endif
}

VistaLoadingProgressUpdate vista_loading_progress_update(VistaLoadingProgressState& state,
    const VistaLoadingProgressQueryResults& query,const VistaLoadingFloatingPoint& context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  auto next=state;
  const float zero=scalar(0),one=scalar(0x3f800000);
  // The query contract supplies the binary32 word after original 647F7D's
  // spill. Re-loading it through x87 would add a conversion at this boundary.
  float map=scalar(query.map_progress_bits);
  float increment=one;
  const float duration=integer(query.duration);
  auto elapsed=unsigned_clock(query.clock-next.previous_clock);
  const float elapsed32=spill(elapsed);
  next.previous_clock=query.clock;
  if(above(duration,zero)) {
    if(!above(elapsed32,zero))elapsed=load(zero);
    increment=spill(divide_extended(elapsed,duration));
  }
  float baseline=scalar(next.baseline_bits);
  if(above(zero,baseline)) {
    baseline=map;
    next.baseline_bits=bits(baseline);
  }
  const float denominator=subtract(one,baseline);
  float normalized;
  if(above_equal(scalar(0x3a83126f),denominator))normalized=one;
  else {
    if(above(baseline,map))map=baseline;
    else if(above(map,one))map=one;
    normalized=divide(subtract(map,baseline),denominator);
    if(above(zero,normalized))normalized=zero;
    else if(!below_equal_or_unordered(normalized,one))normalized=one;
  }
  // 648076 spills target to binary32 but compares its retained x87 value.
  const auto target_value=target(scalar(query.secondary_progress_bits),normalized);
  const float target32=spill(target_value),progress=scalar(next.progress_bits);
  if(!below_or_unordered(target_value,progress)) {
    const float candidate=add(progress,increment);
    next.progress_bits=bits(below_equal_or_unordered(target32,candidate)?target32:candidate);
  }
  state=next;
  return {{VistaLoadingProgressService::path_conversion,VistaLoadingProgressService::map_progress,
      VistaLoadingProgressService::duration,VistaLoadingProgressService::clock,
      VistaLoadingProgressService::secondary_progress}};
#else
  (void)state;(void)query;(void)context;
  throw std::runtime_error("Original Vista loading x87/SSE backend unavailable");
#endif
}

VistaLoadingFadeBegin vista_loading_fade_begin(VistaLoadingFadeState& state,
    std::uint32_t origin,std::uint32_t now,const VistaLoadingFloatingPoint& context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  state.start_seconds_bits=bits(elapsed_seconds(origin,now));
  state.active=1;
  return {{VistaLoadingFadeService::clock}};
#else
  (void)state;(void)origin;(void)now;(void)context;
  throw std::runtime_error("Original Vista loading x87/SSE backend unavailable");
#endif
}

VistaLoadingFadeUpdate vista_loading_fade_update(VistaLoadingFadeState& state,
    std::uint32_t origin,std::uint32_t now,const VistaLoadingFloatingPoint& context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  VistaLoadingFadeUpdate result{bits(elapsed_seconds(origin,now)),false,{},{}};
  if(!state.active)return result;
  const float zero=scalar(0),one=scalar(0x3f800000),duration=scalar(0x3fc00000);
  const float elapsed=scalar(result.elapsed_seconds_bits),start=scalar(state.start_seconds_bits);
  float gain;
  if(above(elapsed,add(duration,start))) {
    result.complete=true;
    gain=zero;
  } else {
    gain=subtract(one,divide(subtract(elapsed,start),duration));
    if(above(zero,gain))gain=zero;
    else if(!below_equal_or_unordered(gain,one))gain=one;
  }
  state.gain_bits=bits(gain);
  result.gain_service_bits=bits(copy(gain));
  result.packed_argb=packed_color(gain);
  return result;
#else
  (void)state;(void)origin;(void)now;(void)context;
  throw std::runtime_error("Original Vista loading x87/SSE backend unavailable");
#endif
}
} // namespace halo2
