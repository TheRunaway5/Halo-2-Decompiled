#pragma once
#include <cstdint>
namespace halo2 {
// Retail playing sound +AE/+B0/+B4/+B8 and fading flag at +4, consumed by
// 12A810. Relative start/end times become absolute on the first update.
struct SoundPlaybackFade {
  std::uint16_t curve = 1;
  float gain_db{};
  std::int32_t start{}, end = -1;
  bool fading = true;
};
float source_sound_playback_fade(SoundPlaybackFade &, std::int32_t time_ms);
} // namespace halo2
