#pragma once
#include <cmath>
#include <cstdint>

namespace halo2 {
// Original22CD48's buffer-flag0 opacity pulse. The UI clock is cumulative;
// alpha and frequency enter the original x87 corridor as float32 values.
inline float frontend_lobby_warning_alpha(float input_alpha,
                                          std::uint32_t ui_clock_ms,
                                          bool warning = true) {
  if (!warning)
    return input_alpha;
  const double phase = double(ui_clock_ms) * double(.003f);
  return float((std::cos(phase) + 1.0) * .5 * double(input_alpha));
}
} // namespace halo2
