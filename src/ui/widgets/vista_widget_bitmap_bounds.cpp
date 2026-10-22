#include "halo2/vista_widget_bitmap_bounds.hpp"
#include <bit>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
std::uint32_t vista_widget_bitmap_bounds(std::uint32_t widget,
    std::uint32_t output,const VistaWidgetBoundsServices& s) {
  // The source reads and writes in this prefix are identical to611703,
  // including the otherwise-unused Z read and Y-before-X truncation.
  vista_widget_short_bounds(widget,output,s);
#if defined(__i386__) || defined(__x86_64__)
  const auto word=[&](std::uint32_t at) {
    return std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(s.read(at,2))));
  };
  const auto value=[](std::uint32_t bits) {
    return _mm_set_ss(std::bit_cast<float>(bits));
  };
  const auto integer=[](std::int32_t v) {
    return _mm_cvtsi32_ss(_mm_setzero_ps(),v);
  };
  const auto left=word(output+2);
  const auto right=word(output+6);
  const auto width=right-left;
  const auto float_left=integer(left);
  const auto top=word(output);
  auto float_width=integer(width);
  float_width=_mm_mul_ss(float_width,value(s.read(widget+0x84,4)));
  const auto bottom=word(output+4);
  float_width=_mm_add_ss(float_width,float_left);
  const auto scaled_right=_mm_cvtt_ss2si(float_width);
  const auto height=bottom-top;
  s.write(output+6,std::uint16_t(scaled_right),2);
  auto float_height=integer(height);
  float_height=_mm_mul_ss(float_height,value(s.read(widget+0x88,4)));
  const auto float_top=integer(top);
  float_height=_mm_add_ss(float_height,float_top);
  const auto scaled_bottom=_mm_cvtt_ss2si(float_height);
  s.write(output+4,std::uint16_t(scaled_bottom),2);
#endif
  return output;
}
} //namespace halo2
