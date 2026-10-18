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
// TU1.5 128E70: sample the current fade through12D320, then start the
// original linear300ms stop request. The next update resolves relative times.
// This only authors playback state; the actual voice owner retires playback.
void source_sound_request_stop_tu15(SoundPlaybackFade &, std::int32_t time_ms);
} // namespace halo2
