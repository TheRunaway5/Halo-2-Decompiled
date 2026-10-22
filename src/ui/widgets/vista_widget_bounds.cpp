#include "halo2/vista_widget_bounds.hpp"
#include "halo2/cache.hpp"
#include <bit>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if defined(__i386__) || defined(__x86_64__)
struct Owner {
  const VistaWidgetBoundsServices& s;
  std::uint32_t r(std::uint32_t at,std::size_t n=4)const{return s.read(at,n);}
  void w(std::uint32_t at,std::uint32_t v,std::size_t n=4)const{s.write(at,v,n);}
  std::int32_t word(std::uint32_t at)const {
    return std::bit_cast<std::int16_t>(std::uint16_t(r(at,2)));
  }
};
__m128 value(std::uint32_t bits){return _mm_set_ss(std::bit_cast<float>(bits));}
std::uint32_t bits(__m128 v){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}
__m128 integer(std::int32_t v){return _mm_cvtsi32_ss(_mm_setzero_ps(),v);}
// Keep original operand order even for two NaN payloads. C++ intrinsic
// arithmetic may be commuted by the optimizer; original scalar SSE may not.
__m128 add(__m128 a,__m128 b){asm volatile("addss %1, %0":"+x"(a):"x"(b));return a;}
__m128 subtract(__m128 a,__m128 b){asm volatile("subss %1, %0":"+x"(a):"x"(b));return a;}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulss %1, %0":"+x"(a):"x"(b));return a;}
__m128 divide(__m128 a,__m128 b){asm volatile("divss %1, %0":"+x"(a):"x"(b));return a;}
bool below(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2, %1; setb %0":"=qm"(out):"x"(a),"x"(b):"cc");return out!=0;}
bool below_equal(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2, %1; setbe %0":"=qm"(out):"x"(a),"x"(b):"cc");return out!=0;}
bool above(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2, %1; seta %0":"=qm"(out):"x"(a),"x"(b):"cc");return out!=0;}
void admit(const VistaWidgetBoundsServices& s) {
  if(!s.read||!s.write)throw FormatError("Vista bounds requires its resident source32 registry");
  std::uint16_t control{};asm volatile("fnstcw %0":"=m"(control));
  if((_mm_getcsr()&0x1f80u)!=0x1f80u || (control&0x3fu)!=0x3fu)
    throw FormatError("Unmasked Vista bounds SSE/x87 exceptions are not admitted");
}
std::uint32_t x87_roundtrip(std::uint32_t raw) {
  std::uint32_t result{};
  asm volatile("flds %1; fstps %0":"=m"(result):"m"(raw):"st");
  return result;
}
struct CachedFloat {std::uint32_t stored{};bool below_epsilon{};};
CachedFloat cached_float(std::uint32_t raw,std::uint32_t epsilon) {
  CachedFloat out;unsigned char below{};
  // Original6096A0 leaves the loaded float in ST0.61D05F stores it without
  // popping, compares epsilon with its absolute value, then pops both values.
  asm volatile("flds %[raw]; fsts %[stored]; fabs; flds %[epsilon]; "
               "fcomip %%st(1), %%st; fstp %%st(0); seta %[below]"
      :[stored]"=m"(out.stored),[below]"=q"(below)
      :[raw]"m"(raw),[epsilon]"m"(epsilon):"st","cc");
  out.below_epsilon=below!=0;return out;
}
void project_point(std::uint32_t (&point)[3],std::uint32_t viewport,const Owner&o) {
  const auto left=o.word(viewport+2);const auto right=o.word(viewport+6);
  const auto half=value(o.r(0x79de3cu));
  const auto top=o.word(viewport);auto width=integer(right-left);
  const auto bottom=o.word(viewport+4);width=multiply(width,half);
  auto height=multiply(integer(bottom-top),half);
  const auto raw=o.r(0xd718f8u);auto one=value(o.r(0x79be18u));
  const auto cached=cached_float(raw,o.r(0x7cfe28u));
  const auto distance=cached.below_epsilon?one:value(cached.stored);
  auto depth=add(value(point[2]),distance);point[2]=bits(depth);
  if(below(depth,value(o.r(0x79de6cu)))) {
    const auto negative=value(o.r(0x79e224u));
    if(!below_equal(depth,negative))depth=negative;
  } else if(!above(depth,one))depth=one;
  auto divisor=divide(one,distance);divisor=multiply(divisor,depth);
  one=divide(one,divisor);point[2]=bits(depth);
  auto scale=multiply(value(o.r(0xe3e424u)),one);
  point[0]=bits(multiply(scale,value(point[0])));
  scale=multiply(value(o.r(0xe3e424u)),one);
  auto x=add(value(point[0]),width);
  auto y=multiply(scale,value(point[1]));point[0]=bits(x);point[1]=bits(y);
  height=subtract(height,y);depth=subtract(depth,distance);
  point[1]=bits(height);point[2]=bits(depth);
}
std::uint32_t project(std::uint32_t rectangle,std::uint32_t z,
    std::uint32_t viewport,const Owner&o) {
  const auto x0=o.r(rectangle);const auto x1=o.r(rectangle+4);
  const auto y0=o.r(rectangle+8);const auto y1=o.r(rectangle+12);
  std::uint32_t first[3]{x0,y0,z},second[3]{x1,y1,z};
  project_point(first,viewport,o);project_point(second,viewport,o);
  o.w(rectangle,first[0]);o.w(rectangle+8,first[1]);
  o.w(rectangle+4,second[0]);o.w(rectangle+12,second[1]);return rectangle;
}
#else
void admit(const VistaWidgetBoundsServices&){throw FormatError("Original Vista bounds SSE/x87 backend unavailable");}
#endif
}

std::uint32_t vista_widget_translate_short_rectangle(std::uint32_t rectangle,
    std::uint32_t dx,std::uint32_t dy,const VistaWidgetBoundsServices&s) {
  if(!s.read||!s.write)throw FormatError("Vista short translation requires its source32 registry");
  for(const auto offset:{2u,6u})s.write(rectangle+offset,std::uint16_t(s.read(rectangle+offset,2)+std::uint16_t(dx)),2);
  for(const auto offset:{0u,4u})s.write(rectangle+offset,std::uint16_t(s.read(rectangle+offset,2)+std::uint16_t(dy)),2);
  return rectangle;
}
std::uint32_t vista_widget_short_bounds(std::uint32_t widget,
    std::uint32_t output,const VistaWidgetBoundsServices&s) {
  admit(s);
#if defined(__i386__) || defined(__x86_64__)
  const Owner o{s};o.w(output,o.r(widget+0x20));o.w(output+4,o.r(widget+0x24));
  const auto x=o.r(widget+0x58);const auto y=o.r(widget+0x5c);(void)o.r(widget+0x60);
  const auto dy=std::uint16_t(_mm_cvtt_ss2si(value(y)));
  const auto dx=std::uint16_t(_mm_cvtt_ss2si(value(x)));
  vista_widget_translate_short_rectangle(output,dx,dy,s);
#else
  (void)widget;
#endif
  return output;
}
std::uint32_t vista_widget_float_bounds(std::uint32_t widget,
    std::uint32_t output,const VistaWidgetBoundsServices&s) {
  admit(s);
#if defined(__i386__) || defined(__x86_64__)
  const Owner o{s};o.w(output,bits(integer(o.word(widget+0x22))));
  const auto right=o.word(widget+0x26);auto x0=value(o.r(output));o.w(output+4,bits(integer(right)));
  o.w(output+8,bits(integer(o.word(widget+0x20))));
  const auto bottom=o.word(widget+0x24);auto y0=value(o.r(output+8));o.w(output+12,bits(integer(bottom)));
  const auto x=value(o.r(widget+0x58));const auto y=value(o.r(widget+0x5c));(void)o.r(widget+0x60);
  x0=add(x0,x);o.w(output,bits(x0));y0=add(y0,y);o.w(output+8,bits(y0));
  auto x1=add(value(o.r(output+4)),x);auto y1=add(value(o.r(output+12)),y);
  o.w(output+4,bits(x1));o.w(output+12,bits(y1));
#else
  (void)widget;
#endif
  return output;
}
std::uint32_t vista_widget_projected_bounds(std::uint32_t widget,
    std::uint32_t viewport,std::uint32_t output,const VistaWidgetBoundsServices&s) {
  admit(s);
#if defined(__i386__) || defined(__x86_64__)
  const Owner o{s};o.w(output,bits(integer(o.word(widget+0x22))));
  const auto top=o.word(widget+0x20);auto x0=value(o.r(output));o.w(output+8,bits(integer(top)));
  const auto right=o.word(widget+0x26);auto y0=value(o.r(output+8));o.w(output+4,bits(integer(right)));
  o.w(output+12,bits(integer(o.word(widget+0x24))));
  const auto x=value(o.r(widget+0x58));const auto y=value(o.r(widget+0x5c));const auto z=o.r(widget+0x60);
  x0=add(x0,x);o.w(output,bits(x0));y0=add(y0,y);o.w(output+8,bits(y0));
  auto x1=add(value(o.r(output+4)),x);auto y1=value(o.r(output+12));
  const auto passed_z=x87_roundtrip(z);y1=add(y1,y);
  o.w(output+4,bits(x1));o.w(output+12,bits(y1));return project(output,passed_z,viewport,o);
#else
  (void)widget;(void)viewport;return output;
#endif
}
std::uint32_t vista_widget_project_float_rectangle(std::uint32_t rectangle,
    std::uint32_t z,std::uint32_t viewport,const VistaWidgetBoundsServices&s) {
  admit(s);
#if defined(__i386__) || defined(__x86_64__)
  return project(rectangle,z,viewport,Owner{s});
#else
  (void)z;(void)viewport;return rectangle;
#endif
}
} // namespace halo2
