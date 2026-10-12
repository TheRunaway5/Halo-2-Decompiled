#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
// Import these words from the selected original image. Font assets and label
// payloads remain caller-owned; this module contains neither.
struct FullScoreboardPaintConstants {
  std::uint32_t bright_rgb_threshold{}, pixel_scale{};
  std::uint32_t pulse_phase{}, pulse_one{}, pulse_half{};
  std::array<std::uint32_t, 4> shadow{}; // A,R,G,B
};
} // namespace halo2
