#pragma once
// Internal: original SSE/x87 steps under the live world's control words.
// Each x86 operation loads the control, operates and restores inside one
// asm volatile block, so no compiler motion separates it from its mode.
#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>
// Floating-point steps execute the original instruction under the world's
// live MXCSR/x87 control word inside one asm block (load control, operate,
// restore), so no compiler motion can separate an operation from its mode.
// Other targets fall back to dynamic rounding with -frounding-math.
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#define HALO2_NATIVE_SCRIPT_X86_FP 1
#endif
namespace halo2::native_campaign_script_fp {
#if HALO2_NATIVE_SCRIPT_X86_FP
// Exceptions stay masked; reserved MXCSR bits are never loaded.
inline std::uint32_t sse_control(std::uint32_t mxcsr){return (mxcsr|0x1f80u)&0xffc0u;}
inline std::uint16_t x87_control(std::uint16_t control){return std::uint16_t(control|0x3fu);}
inline float mulss(float a,float b,std::uint32_t mxcsr) {
  const auto mode=sse_control(mxcsr);std::uint32_t saved{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\tmulss %3, %0\n\tldmxcsr %1"
      :"+x"(a),"+m"(saved):"m"(mode),"x"(b));
  return a;
}
inline float addss(float a,float b,std::uint32_t mxcsr) {
  const auto mode=sse_control(mxcsr);std::uint32_t saved{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\taddss %3, %0\n\tldmxcsr %1"
      :"+x"(a),"+m"(saved):"m"(mode),"x"(b));
  return a;
}
inline float subss(float a,float b,std::uint32_t mxcsr) {
  const auto mode=sse_control(mxcsr);std::uint32_t saved{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\tsubss %3, %0\n\tldmxcsr %1"
      :"+x"(a),"+m"(saved):"m"(mode),"x"(b));
  return a;
}
inline float divss(float a,float b,std::uint32_t mxcsr) {
  const auto mode=sse_control(mxcsr);std::uint32_t saved{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\tdivss %3, %0\n\tldmxcsr %1"
      :"+x"(a),"+m"(saved):"m"(mode),"x"(b));
  return a;
}
inline float cvtsi2ss(std::int32_t value,std::uint32_t mxcsr) {
  const auto mode=sse_control(mxcsr);std::uint32_t saved{};float result=0.f;
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\tcvtsi2ssl %3, %0\n\tldmxcsr %1"
      :"+x"(result),"+m"(saved):"m"(mode),"m"(value));
  return result;
}
// FLD dword then FISTP dword; masked invalid yields integer indefinite.
inline std::uint32_t fistp32(float value,std::uint16_t control) {
  const auto mode=x87_control(control);std::uint16_t saved{};std::int32_t result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfistpl %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(value):"st");
  return std::uint32_t(result);
}
// 2AC789..2AC7D2: FLD upper, FLD lower, FILD draw, FMUL unit, FXCH st2,
// FSUB st0,st1, FMULP st2,st0, FXCH st1, FADD st0,st1, FSTP dword, FSTP st0.
inline float random_interval(std::uint32_t draw,float lower,float upper,float unit,std::uint16_t control) {
  const auto mode=x87_control(control);std::uint16_t saved{};const auto integer=std::int32_t(draw);float result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\t"
      "flds %3\n\tflds %4\n\tfildl %5\n\tfmuls %6\n\t"
      "fxch %%st(2)\n\tfsub %%st(1), %%st\n\tfmulp %%st, %%st(2)\n\t"
      "fxch %%st(1)\n\tfadd %%st(1), %%st\n\tfstps %0\n\tfstp %%st(0)\n\t"
      "fldcw %1"
      :"=m"(result),"+m"(saved)
      :"m"(mode),"m"(upper),"m"(lower),"m"(integer),"m"(unit)
      :"st","st(1)","st(2)");
  return result;
}
#else
class Rounding {
public:
  explicit Rounding(unsigned control) : previous_(std::fegetround()) {
    static constexpr std::array modes{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
    if(std::fesetround(modes[control&3u]))throw std::runtime_error("Native script rounding control unavailable");
  }
  ~Rounding(){std::fesetround(previous_);}
  Rounding(const Rounding&)=delete;
  Rounding& operator=(const Rounding&)=delete;
private:
  int previous_;
};
inline float sse_input(float value,std::uint32_t mxcsr) {
  if((mxcsr&0x40u) && std::fpclassify(value)==FP_SUBNORMAL)return std::copysign(0.f,value);
  return value;
}
inline float sse_output(float value,std::uint32_t mxcsr) {
  if((mxcsr&0x8000u) && (mxcsr&0x800u) && std::fpclassify(value)==FP_SUBNORMAL)return std::copysign(0.f,value);
  return value;
}
inline float mulss(float a,float b,std::uint32_t mxcsr) {
  const Rounding rounding((mxcsr>>13)&3u);
  volatile float left=sse_input(a,mxcsr),right=sse_input(b,mxcsr);
  volatile float product=left*right;
  return sse_output(product,mxcsr);
}
inline float addss(float a,float b,std::uint32_t mxcsr) {
  const Rounding rounding((mxcsr>>13)&3u);
  volatile float left=sse_input(a,mxcsr),right=sse_input(b,mxcsr);
  volatile float sum=left+right;
  return sse_output(sum,mxcsr);
}
inline float subss(float a,float b,std::uint32_t mxcsr) {
  const Rounding rounding((mxcsr>>13)&3u);
  volatile float left=sse_input(a,mxcsr),right=sse_input(b,mxcsr);
  volatile float difference=left-right;
  return sse_output(difference,mxcsr);
}
inline float divss(float a,float b,std::uint32_t mxcsr) {
  const Rounding rounding((mxcsr>>13)&3u);
  volatile float left=sse_input(a,mxcsr),right=sse_input(b,mxcsr);
  volatile float quotient=left/right;
  return sse_output(quotient,mxcsr);
}
inline float cvtsi2ss(std::int32_t value,std::uint32_t mxcsr) {
  const Rounding rounding((mxcsr>>13)&3u);
  volatile std::int32_t input=value;
  volatile float result=float(input);
  return result;
}
inline std::uint32_t fistp32(float value,std::uint16_t control) {
  const long double extended=value;
  if(std::isnan(extended))return 0x80000000u;
  long double rounded;
  switch((control>>10)&3u) {
  case 0: {
    rounded=std::floor(extended);const long double fraction=extended-rounded;
    if(fraction>0.5L || (fraction==0.5L && std::fmod(rounded,2.L)!=0))rounded+=1;
    break;
  }
  case 1:rounded=std::floor(extended);break;
  case 2:rounded=std::ceil(extended);break;
  default:rounded=std::trunc(extended);break;
  }
  if(rounded< -2147483648.L || rounded>2147483647.L)return 0x80000000u;
  return std::uint32_t(std::int32_t(rounded));
}
inline float random_interval(std::uint32_t draw,float lower,float upper,float unit,std::uint16_t control) {
  const Rounding rounding((control>>10)&3u);
  switch((control>>8)&3u) {
  case 2: {
    volatile double scaled=double(draw)*double(unit);volatile double width=double(upper)-double(lower);
    volatile double product=scaled*width;volatile double sum=product+double(lower);volatile float result=float(sum);
    return result;
  }
  case 3: {
    using X=long double;
    volatile X scaled=X(draw)*X(unit);volatile X width=X(upper)-X(lower);
    volatile X product=scaled*width;volatile X sum=product+X(lower);volatile float result=float(sum);
    return result;
  }
  default: {
    volatile float scaled=float(draw)*unit;volatile float width=upper-lower;
    volatile float product=scaled*width;volatile float sum=product+lower;
    return sum;
  }
  }
}
#endif

// COMISS/UCOMISS operand under MXCSR.DAZ: denormals act as signed zero.
inline float sse_compared(std::uint32_t bits,std::uint32_t mxcsr) {
  if((mxcsr&0x40u) && !(bits&0x7f800000u))bits&=0x80000000u;
  return std::bit_cast<float>(bits);
}
// FLD qword clock; FST dword; FADD dword addend; FSTP dword.
inline std::pair<float,float> x87_clock_store_add(double clock,float addend,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=x87_control(control);std::uint16_t saved{};float stored{},sum{};
  asm volatile("fnstcw %2\n\tfldcw %3\n\tfldl %4\n\tfsts %0\n\tfadds %5\n\tfstps %1\n\tfldcw %2"
      :"=m"(stored),"=m"(sum),"+m"(saved):"m"(mode),"m"(clock),"m"(addend):"st");
  return {stored,sum};
#else
  const Rounding rounding((control>>10)&3u);
  volatile float stored=float(clock);volatile double total=clock+double(addend);volatile float sum=float(total);
  return {stored,sum};
#endif
}
}
