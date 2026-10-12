#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace halo2 {
// Original 12B0E0 clamps the frame delta; 1479C3 multiplies its stored float
// by 1000 in the x87 double-precision mode and 322B0C truncates before adding.
inline std::uint32_t frontend_ui_clock_advance(std::uint32_t clock,
                                              float delta_seconds) {
  return clock + static_cast<std::uint32_t>(
      double(std::clamp(delta_seconds, 0.f, 10.f)) * 1000.0);
}

// 237D30: unsigned UI clock, original float coefficient, x87 cosine product,
// float store followed by nearest-even integer conversion (original FPCW).
inline unsigned frontend_keyboard_caret_alpha(std::uint32_t clock,
                                               float widget_alpha) {
  const float value = float((std::cos(double(clock) * double(.003f)) + 1.0) *
                            .5 * double(widget_alpha) * 255.0);
  return static_cast<unsigned>(std::nearbyint(value));
}
} // namespace halo2
