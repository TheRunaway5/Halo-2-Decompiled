#include "halo2/source_king_boundary.hpp"
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
struct Scope {
  std::fenv_t environment;
  unsigned mxcsr;
  explicit Scope(SourceKingBoundaryFloatingPoint fp):mxcsr(_mm_getcsr()) {
    if((fp.x87_control_word&0x3fu)!=0x3fu || (fp.mxcsr&0x1f80u)!=0x1f80u ||
        ((fp.x87_control_word>>8)&3u)==1u)
      throw std::invalid_argument("King boundary requires masked exceptions and a defined x87 precision");
    if(std::fegetenv(&environment))throw std::runtime_error("King boundary floating point environment unavailable");
    asm volatile("fldcw %0"::"m"(fp.x87_control_word):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Scope(){std::fesetenv(&environment);_mm_setcsr(mxcsr);}
};
// Preserve actual x87 stack operations, precision, rounding and binary32
// stores. The common distance sequence is literal19F68A/19F970/19FBDF.
#define H2_KING_X87_DISTANCE \
  "flds %2\n\tfsubs %4\n\tflds %3\n\tfsubs %5\n\t" \
  "fld %%st(0)\n\tfmul %%st(1),%%st\n\tfld %%st(2)\n\t" \
  "fmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\t" \
  "fstp %%st(2)\n\tfstp %%st(0)\n\t"
struct Rounded {float stored;long double live;};
Rounded distance_minus(const std::array<float,2>& point,const std::array<float,3>& camera,float radius) {
  Rounded out;
  asm volatile(H2_KING_X87_DISTANCE "fsubs %6\n\tfsts %0\n\tfstpt %1"
    :"=m"(out.stored),"=m"(out.live)
    :"m"(point[0]),"m"(point[1]),"m"(camera[0]),"m"(camera[1]),"m"(radius)
    :"st","st(1)","st(2)","st(3)","memory");return out;
}
Rounded opacity(const std::array<float,2>& point,const std::array<float,3>& camera,float outer,float denominator) {
  Rounded out;
  asm volatile(H2_KING_X87_DISTANCE "fsubrs %6\n\tfdivs %7\n\tfsts %0\n\tfstpt %1"
    :"=m"(out.stored),"=m"(out.live)
    :"m"(point[0]),"m"(point[1]),"m"(camera[0]),"m"(camera[1]),"m"(outer),"m"(denominator)
    :"st","st(1)","st(2)","st(3)","memory");return out;
}
float next_u(const std::array<float,2>& previous,const std::array<float,2>& point,float scale,float u) {
  float out;long double unused;
  asm volatile(H2_KING_X87_DISTANCE "fmuls %6\n\tfsubrs %7\n\tfstps %0"
    :"=m"(out),"=m"(unused)
    :"m"(previous[0]),"m"(previous[1]),"m"(point[0]),"m"(point[1]),"m"(scale),"m"(u)
    :"st","st(1)","st(2)","st(3)","memory");return out;
}
#undef H2_KING_X87_DISTANCE
std::int32_t fistp(float value) {
  std::int32_t out;asm volatile("flds %1\n\tfistpl %0":"=m"(out):"m"(value):"st","memory");return out;
}
float scalar(__m128 value){return _mm_cvtss_f32(value);}
#endif
}
std::optional<SourceKingBoundary> source_king_boundary(const SourceKingState& state,
    const std::array<float,3>& camera,const std::array<float,3>& rgb,
    std::int32_t tick,std::int16_t rate,SourceKingBoundaryFloatingPoint fp) {
  if(state.marker_count<4 || (state.marker_count&1))return {};
  if(!(state.radius>0) || !std::isfinite(state.radius) || !(state.perimeter>0) || !std::isfinite(state.perimeter) || !rate)
    throw std::invalid_argument("King boundary requires finite positive authored geometry and a nonzero source clock rate");
  for(const auto& point:state.polygon)for(float value:point)if(!std::isfinite(value))
    throw std::invalid_argument("King boundary has unavailable authored polygon coordinates");
  for(const auto& vector:{state.center,camera,rgb})for(float value:vector)if(!std::isfinite(value))
    throw std::invalid_argument("King boundary has unavailable center/view/color inputs");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Scope scope(fp);const auto radius=_mm_set_ss(state.radius),one=_mm_set_ss(1.f);
  const auto limit=_mm_mul_ss(_mm_set_ss(1.5f),radius);
  const auto distance=distance_minus({state.center[0],state.center[1]},camera,state.radius);
  auto d=distance.live<0 ? _mm_setzero_ps() : _mm_set_ss(distance.stored);
  if(_mm_comigt_ss(d,limit))d=limit;
  const auto fade=_mm_sub_ss(one,_mm_div_ss(d,limit));
  if(_mm_comile_ss(fade,_mm_setzero_ps()))return {};
  SourceKingBoundary out;
  out.inner=scalar(_mm_mul_ss(_mm_mul_ss(fade,radius),_mm_set_ss(2.f)));
  const auto adjusted=_mm_sub_ss(radius,_mm_set_ss(std::bit_cast<float>(0x38d1b717u)));
  out.outer=scalar(_mm_add_ss(_mm_set_ss(out.inner),adjusted));
  constexpr float height=std::bit_cast<float>(0x3f4ccccdu);
  const float estimate=scalar(_mm_div_ss(_mm_set_ss(state.perimeter),
      _mm_mul_ss(_mm_set_ss(height),_mm_set_ss(std::bit_cast<float>(0x3f99999au)))));
  out.repeats=fistp(estimate);
  if(out.repeats<2)out.repeats=2;else out.repeats=fistp(estimate);
  out.uv_scale=scalar(_mm_div_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),out.repeats),_mm_set_ss(state.perimeter)));
  const int period=int(rate)*2;
  out.initial_u=scalar(_mm_div_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),tick%period),_mm_cvtsi32_ss(_mm_setzero_ps(),period)));
  const auto denominator=scalar(_mm_sub_ss(_mm_set_ss(out.outer),_mm_set_ss(out.inner)));
  float u=out.initial_u;
  const float upper=scalar(_mm_add_ss(_mm_set_ss(state.center[2]),_mm_set_ss(height)));
  for(unsigned j=0;j<=32;++j) {
    const auto i=j==32 ? 0 : j;const auto& point=state.polygon[i];
    const auto alpha=opacity(point,camera,out.outer,denominator);
    float a=alpha.live<0 ? 0.f : alpha.stored;if(a>1)a=1;
    const std::array<float,4> rgba{rgb[0],rgb[1],rgb[2],a};
    out.strip[j*2]={{point[0],point[1],state.center[2],1.f},{u,1.f},rgba};
    out.strip[j*2+1]={{point[0],point[1],upper,1.f},{u,0.f},rgba};
    u=next_u(state.polygon[i==0 ? 31 : j-1],point,out.uv_scale,u);
  }
  return out;
#else
  (void)tick;(void)fp;throw std::runtime_error("King boundary source x87 arithmetic backend unavailable");
#endif
}
} // namespace halo2
