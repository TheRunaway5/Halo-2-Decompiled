#include "halo2/model.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
std::int16_t animation_ready_ticks(const AnimationClip &clip,
                                  std::int16_t clock_rate) {
  int selected = std::bit_cast<std::int16_t>(std::uint16_t(clip.frame_count));
  for (const auto &event : clip.frame_events)
    if (event.type == 4) {
      if (event.frame != -1 && selected > event.frame) selected = event.frame;
      break;
    }
  if (selected == -1) return -1;
  if (selected <= 0) return 0;
  const float seconds = float(selected) * std::bit_cast<float>(0x3d088889u);
  const float ticks = seconds * float(clock_rate);
  // FISTP under the oracle's recorded CW027F. Explicit tie handling avoids
  // inheriting a caller's host integer-rounding convention.
  auto rounded = std::int32_t(std::floor(double(ticks)));
  const double fraction = double(ticks) - rounded;
  if (fraction > .5 || (fraction == .5 && (rounded & 1))) ++rounded;
  const auto result = std::bit_cast<std::int16_t>(std::uint16_t(rounded));
  return result > 1 ? result : 1;
}
} // namespace halo2
