#pragma once
#include <bit>
#include <cfenv>
#include <cstdint>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
// Actual TU1.5 2CF0A9 initializes these 515E10/14/18 fields. Query values
// remain supplied by their real map-resource, UI-global and clock owners.
// This kernel neither invents map progress nor dismisses a loading interval.
struct OriginalLoadingProgress {
  std::uint32_t baseline_bits=0xbf800000u,progress_bits{},previous_clock{};
  bool operator==(const OriginalLoadingProgress&)const=default;
};
struct OriginalLoadingProgressQueries {
  std::uint32_t map_progress_bits{};
  std::int32_t duration{};
  std::uint32_t clock{},secondary_progress_bits{};
};
struct OriginalLoadingFloatingPoint {
  std::uint16_t x87_control_word;
  std::uint32_t mxcsr;
};
// Original288C0's retained4C61C4/4C61C8 input/filter bank, reset by28960.
struct OriginalLoadingFilter {
  std::uint32_t previous_bits{},delta_bits{};
  bool operator==(const OriginalLoadingFilter&)const=default;
};

namespace original_loading_detail {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class FloatingPointScope {
  std::fenv_t previous_;
  unsigned mxcsr_;
public:
  explicit FloatingPointScope(const OriginalLoadingFloatingPoint&context):mxcsr_(_mm_getcsr()) {
    if((context.x87_control_word&0x3f)!=0x3f || (context.x87_control_word&0x300)==0x100 ||
        (context.mxcsr&0x1f80)!=0x1f80 || (context.mxcsr&0xffff0000))
      throw std::runtime_error("Unsupported original Xbox loading floating-point context");
    if(std::fegetenv(&previous_)!=0)throw std::runtime_error("Original loading floating-point owner unavailable");
    _mm_setcsr(context.mxcsr);
    asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope(){std::fesetenv(&previous_);_mm_setcsr(mxcsr_);}
};
inline float scalar(std::uint32_t v){return std::bit_cast<float>(v);}
inline std::uint32_t bits(float v){return std::bit_cast<std::uint32_t>(v);}
inline float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
inline float sub(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
inline float mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
inline float divide(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}
inline bool above(float a,float b){unsigned char out;asm volatile("comiss %2,%1; seta %0":"=qm"(out):"x"(a),"x"(b):"cc");return out;}
inline bool above_equal(float a,float b){unsigned char out;asm volatile("comiss %2,%1; setae %0":"=qm"(out):"x"(a),"x"(b):"cc");return out;}
inline bool below(float a,float b){unsigned char out;asm volatile("comiss %2,%1; setb %0":"=qm"(out):"x"(a),"x"(b):"cc");return out;}
inline bool below_equal(float a,float b){unsigned char out;asm volatile("comiss %2,%1; setbe %0":"=qm"(out):"x"(a),"x"(b):"cc");return out;}
inline float clock_increment(std::uint32_t elapsed,float duration) {
  const auto value=std::bit_cast<std::int32_t>(elapsed);const float wrap=scalar(0x4f800000u);
  long double extended;
  asm volatile("fildl %1; fstpt %0":"=m"(extended):"m"(value):"st");
  if(value<0)asm volatile("fldt %1; fadds %2; fstpt %0":"=m"(extended):"m"(extended),"m"(wrap):"st");
  float out;
  asm volatile("fldt %1; fdivs %2; fstps %0":"=m"(out):"m"(extended),"m"(duration):"st");
  return out;
}
#endif
}

// Literal TU1.5 2CF16C arithmetic after1258E0 map query,23C68E duration,
//33B590 clock and23C6CB secondary query. Unlike the PC body, the target's
//0.3/0.7 products, addition and comparisons are SSE binary32 operations.
inline void original_loading_progress_update(OriginalLoadingProgress&state,
    const OriginalLoadingProgressQueries&query,const OriginalLoadingFloatingPoint&context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  using namespace original_loading_detail;
  FloatingPointScope scope(context);auto next=state;
  const float zero=scalar(0),one=scalar(0x3f800000u);
  float map=scalar(query.map_progress_bits),increment=one;
  const float duration=_mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),query.duration));
  if(above(duration,zero))increment=clock_increment(query.clock-next.previous_clock,duration);
  next.previous_clock=query.clock;
  float baseline=scalar(next.baseline_bits);
  if(above(zero,baseline)){baseline=map;next.baseline_bits=bits(baseline);}
  const float denominator=sub(one,baseline);
  float normalized;
  if(above_equal(scalar(0x3a83126fu),denominator))normalized=one;
  else {
    if(above(baseline,map))map=baseline;
    else if(above(map,one))map=one;
    normalized=divide(sub(map,baseline),denominator);
    if(above(zero,normalized))normalized=zero;
    else if(!below_equal(normalized,one))normalized=one;
  }
  const float target=add(mul(scalar(query.secondary_progress_bits),scalar(0x3e99999au)),
      mul(normalized,scalar(0x3f333333u)));
  const float previous=scalar(next.progress_bits);
  if(!below(target,previous)) {
    const float candidate=add(previous,increment);
    next.progress_bits=bits(below_equal(target,candidate)?target:candidate);
  }
  state=next;
#else
  (void)state;(void)query;(void)context;
  throw std::runtime_error("Original Xbox loading arithmetic backend unavailable");
#endif
}

// Complete unchanged288C0 filter decisions, including COMISS unordered
// branches and exact literal weights0.0799999833/0.9200000167.
inline std::uint32_t original_loading_filtered_progress(OriginalLoadingFilter&state,
    std::uint32_t input,const OriginalLoadingFloatingPoint&context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  using namespace original_loading_detail;
  FloatingPointScope scope(context);const float value=scalar(input),previous=scalar(state.previous_bits),zero=scalar(0),one=scalar(0x3f800000u);
  float change=sub(value,previous);if(above(zero,change))change=zero;
  const float delta=add(mul(scalar(state.delta_bits),scalar(0x3f6b851fu)),mul(change,scalar(0x3da3d708u)));
  state.delta_bits=bits(delta);float out=add(delta,previous);
  if(!below(value,scalar(0x3f7fbe77u)))return bits(one);
  if(above(zero,out))out=zero;
  else if(!below_equal(out,one))out=one;
  return bits(out);
#else
  (void)state;(void)input;(void)context;
  throw std::runtime_error("Original Xbox loading arithmetic backend unavailable");
#endif
}
} // namespace halo2
