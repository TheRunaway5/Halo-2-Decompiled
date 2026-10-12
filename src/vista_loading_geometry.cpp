#include "halo2/vista_loading_geometry.hpp"

#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
float f(std::uint32_t word) { return std::bit_cast<float>(word); }
std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }
class FloatingPointScope {
  std::fenv_t saved_;
  unsigned mxcsr_;
public:
  explicit FloatingPointScope(const VistaLoadingFloatingPoint& context):mxcsr_(_mm_getcsr()) {
    // Actual observed ordinary loading thread uses this control context.
    if(context.x87_control_word!=0x027f || (context.mxcsr&~0x3fu)!=0x1f80)
      throw std::runtime_error("Original Vista loading geometry FP context unverified");
    if(std::fegetenv(&saved_)!=0)
      throw std::runtime_error("Original Vista loading geometry FP context unavailable");
    _mm_setcsr(context.mxcsr);
    asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope() { std::fesetenv(&saved_);_mm_setcsr(mxcsr_); }
};
float add(float a,float b) { return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float sub(float a,float b) { return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float mul(float a,float b) { return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float div(float a,float b) { return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float integer(std::uint32_t a) { return _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),std::bit_cast<std::int32_t>(a))); }
long double extended_add(long double a,long double b) {
  long double result;
  asm volatile("fldt %1; fldt %2; faddp; fstpt %0":"=m"(result):"m"(a),"m"(b):"st","st(1)");
  return result;
}
long double extended_sub(long double a,long double b) {
  long double result;
  asm volatile("fldt %1; fldt %2; fsubrp; fstpt %0":"=m"(result):"m"(a),"m"(b):"st","st(1)");
  return result;
}
long double extended_mul(long double a,long double b) {
  long double result;
  asm volatile("fldt %1; fldt %2; fmulp; fstpt %0":"=m"(result):"m"(a),"m"(b):"st","st(1)");
  return result;
}
float spill(long double a) {
  float result;
  asm volatile("fldt %1; fstps %0":"=m"(result):"m"(a):"st");
  return result;
}
struct Trig { long double sine,cosine;float sine32,cosine32; };
Trig trig(float angle) {
  Trig t;
  asm volatile("flds %4; fld %%st(0); fcos; fsts %3; fstpt %1; fsin; fsts %2; fstpt %0"
      :"=m"(t.sine),"=m"(t.cosine),"=m"(t.sine32),"=m"(t.cosine32):"m"(angle):"st","st(1)");
  return t;
}
std::array<std::array<float,2>,4> rotated_uv(float angle) {
  const auto t=trig(angle);
  const float negative_cosine=mul(t.cosine32,-1.0f),negative_sine=mul(t.sine32,-1.0f);
  auto coordinate=[](float a,float b) { return mul(add(sub(a,b),1.0f),0.5f); };
  auto extended_coordinate=[](long double a,long double b) {
    return spill(extended_mul(extended_add(extended_sub(a,b),1.0L),0.5L));
  };
  return {{{coordinate(negative_cosine,negative_sine),coordinate(negative_cosine,t.sine32)},
      {coordinate(t.cosine32,negative_sine),extended_coordinate(t.sine,t.cosine)},
      {extended_coordinate(t.cosine,t.sine),spill(extended_mul(extended_add(extended_add(t.cosine,t.sine),1.0L),0.5L))},
      {coordinate(negative_cosine,t.sine32),coordinate(t.cosine32,t.sine32)}}};
}

// Bounded positive finite input branch of original68AE11→68AEB2→6939D7.
// The caller's ratio is first spilled to double, as in68AE51, before FYL2X.
// This intentionally does not substitute std::pow or x*x.
long double original_square(float elapsed,float threshold) {
  if(elapsed==0.0f)return 0.0L; // Actual68AF9B zero-base/positive-exponent path.
  double ratio;
  asm volatile("flds %1; fdivs %2; fstpl %0":"=m"(ratio):"m"(elapsed),"m"(threshold):"st");
  const double exponent=2.0;
  long double result;
  asm volatile("fldl %2; fldl %1; fyl2x; fld %%st(0); frndint; fsub %%st(0),%%st(1); fxch %%st(1); fchs; f2xm1; fld1; faddp; fscale; fstp %%st(1); fstpt %0"
      :"=m"(result):"m"(ratio),"m"(exponent):"st","st(1)");
  return result;
}
float spinner_size(float elapsed,float index) {
  const float period=f(0x41700000),tau=f(0x40c90fdb),half=f(0x3f000000);
  const float upper=f(0x3fcccccd),lower=f(0x3f800000),scale=f(0x3f3851eb);
  float result;
  asm volatile("flds %1; fdivs %2; fmuls %3; fsin; fadds %5; fmuls %4; flds %6; flds %5; fsubr %%st(0),%%st(1); fxch %%st(2); fmulp; faddp; fmuls %7; fdivs %8; fstps %0"
      :"=m"(result):"m"(elapsed),"m"(period),"m"(tau),"m"(half),"m"(lower),"m"(upper),"m"(scale),"m"(index)
      :"st","st(1)","st(2)");
  return result;
}
float sweep_phase(float elapsed) {
  const float thousand=f(0x447a0000);
  std::uint16_t cw,chop;
  std::int64_t integer_clock;
  asm volatile("fnstcw %0":"=m"(cw));
  chop=cw|0xc00;
  asm volatile("flds %1; fmuls %2; fldcw %3; fistpll %0; fldcw %4"
      :"=m"(integer_clock):"m"(elapsed),"m"(thousand),"m"(chop),"m"(cw):"st","memory");
  auto remainder=std::uint32_t(integer_clock)%30000u;
  const bool reverse=remainder>=15000;
  if(reverse)remainder-=15000;
  const std::int32_t a=std::int32_t(remainder),b=15000;
  float result;
  if(reverse)
    asm volatile("fildl %1; fidivl %2; fld1; fsubp; fstps %0":"=m"(result):"m"(a),"m"(b):"st","st(1)");
  else
    asm volatile("fildl %1; fidivl %2; fstps %0":"=m"(result):"m"(a),"m"(b):"st");
  return result;
}
float sweep_secondary(float phase) {
  const float pi=f(0x40490fdb),one=f(0x3f800000),offset=f(0xbe800000);
  float result;
  asm volatile("flds %1; fmuls %2; fsin; fld1; fsubp; fmuls %3; fadds %4; fstps %0"
      :"=m"(result):"m"(phase),"m"(pi),"m"(one),"m"(offset):"st","st(1)");
  return result;
}
VistaLoadingVertex vertex(float x,float y,float u,float v,std::uint32_t color) {
  return {{bits(x),bits(y)},{bits(u),bits(v)},{0,0},color};
}
VistaLoadingDraw quad(std::uint32_t shader,std::uint32_t texture,VistaLoadingBlend blend,
    float left=0,float top=0,float right=1,float bottom=1,float uleft=0,float uright=1,
    std::uint32_t color=0xffffffffu) {
  return {5,shader,texture,blend,{},{{vertex(left,top,uleft,0,color),vertex(left,bottom,uleft,1,color),
      vertex(right,top,uright,0,color),vertex(right,bottom,uright,1,color)}}};
}
#endif
}

std::vector<VistaLoadingDraw> vista_loading_geometry(std::uint32_t elapsed_bits,
    std::uint32_t width,std::uint32_t height,std::uint32_t progress_bits,
    std::optional<std::uint32_t> fade,const VistaLoadingFloatingPoint& context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope fp(context);
  const float elapsed=f(elapsed_bits);
  if(!width || !height || width>32767 || height>32767 || !std::isfinite(elapsed) || elapsed<0.0f)
    throw std::runtime_error("Original Vista loading geometry viewport/clock domain unsupported");
  const float aspect=div(integer(width),integer(height));
  std::vector<VistaLoadingDraw> result;
  result.reserve(fade?12:11);
  result.push_back(quad(0,0,VistaLoadingBlend::one_one));
  for(std::uint32_t i=0;i<3;++i) {
    const float index=integer(i+1),threshold=add(mul(index,0.5f),1.0f);
    float rotation;
    if(threshold>elapsed)
      rotation=spill(extended_mul(extended_mul(original_square(elapsed,threshold),threshold),f(0x3fa66666)));
    else rotation=mul(f(0x3fa66666),elapsed);
    const float size=spinner_size(elapsed,index),half_size=mul(size,0.5f),half_width=div(half_size,aspect);
    const float left=sub(0.5f,half_width),right=add(half_width,0.5f),top=sub(0.5f,half_size),bottom=add(half_size,0.5f);
    for(std::uint32_t layer=0;layer<2;++layer) {
      const float angle=mul(mul(mul(rotation,f(layer?0xbe567750:0x3e00adfd)),index),f(0x3f8ccccd));
      const auto uv=rotated_uv(angle);
      VistaLoadingDraw draw{6,1,3+layer,VistaLoadingBlend::one_one,
          std::array<std::uint32_t,4>{layer?0x3e888889u:0x3e50d0d1u,0,0,0},
          {{vertex(left,top,uv[0][0],uv[0][1],0xff20258c),vertex(left,bottom,uv[1][0],uv[1][1],0xff20258c),
            vertex(right,bottom,uv[2][0],uv[2][1],0xff20258c),vertex(right,top,uv[3][0],uv[3][1],0xff20258c)}}};
      result.push_back(draw);
    }
  }
  const float phase=sweep_phase(elapsed),inverse=sub(1.0f,phase),unit=div(1.0f,f(0x45000000));
  const float left=mul(mul(inverse,f(0x44280000)),unit),right=mul(mul(add(inverse,1.0f),f(0x44280000)),unit);
  auto sweep=quad(2,5,VistaLoadingBlend::one_one,0,0,1,1,left,right);
  sweep.pixel_constant0=std::array<std::uint32_t,4>{0x3fe00000u,progress_bits,0,0};
  result.push_back(sweep);
  sweep.blend=VistaLoadingBlend::destination_color_zero;
  const float base=sub(mul(phase,2.0f),1.0f),squared=mul(base,base),curve=sub(1.0f,squared),curve_squared=mul(curve,curve);
  sweep.pixel_constant0=std::array<std::uint32_t,4>{bits(sub(1.0f,mul(curve_squared,f(0x3f28f5c3)))),bits(sweep_secondary(phase)),0,0};
  result.push_back(sweep);
  result.push_back(quad(0,11,VistaLoadingBlend::destination_color_zero));
  result.push_back(quad(0,1,VistaLoadingBlend::source_alpha_inverse_source_alpha,
      f(0x3dcccccd),f(0x3eddddde),f(0x3f666666),f(0x3f111111)));
  if(fade)result.push_back(quad(3,1,VistaLoadingBlend::destination_color_zero,0,0,1,1,0,1,*fade));
  return result;
#else
  (void)elapsed_bits;(void)width;(void)height;(void)progress_bits;(void)fade;(void)context;
  throw std::runtime_error("Original Vista loading geometry x87/SSE backend unavailable");
#endif
}
} // namespace halo2
